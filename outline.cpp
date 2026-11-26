#include "stdafx.h"
#include "outline.h"
#include "theme.h"
#include "lexerStyles.h"
#include "shared/platform.h"

void COutline::Create(HWND hParent, HFONT hFont) {
	INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_TREEVIEW_CLASSES};
	InitCommonControlsEx(&icc);
	m_hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
			WS_CHILD | WS_TABSTOP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_NOHSCROLL, // only the vertical scrollbar
			0, 0, 0, 0, hParent, nullptr, g_hInst, nullptr);
	if( hFont )
		::SendMessage(m_hWnd, WM_SETFONT, (WPARAM)hFont, FALSE);
	LOGFONT lf{}; // the comments, the template lines: bold, italic, bold italic
	if( ::GetObject(hFont ? hFont : (HFONT)::GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf) ) {
		LOGFONT lfBold = lf, lfItalic = lf, lfBoldItalic = lf;
		lfBold.lfWeight = FW_BOLD;
		lfItalic.lfItalic = 1;
		lfBoldItalic.lfWeight = FW_BOLD;
		lfBoldItalic.lfItalic = 1;
		m_hFonts[1] = CreateFontIndirect(&lfBold);
		m_hFonts[2] = CreateFontIndirect(&lfItalic);
		m_hFonts[3] = CreateFontIndirect(&lfBoldItalic);
	}
	applyTheme();
}

void COutline::applyTheme() {
	g_theme.applySci(m_hWnd); // scrollbars + no 3D border in dark mode
	bool bDark = g_theme.isDark();
	TreeView_SetBkColor(m_hWnd, bDark ? CTheme::clrEditorBack : (COLORREF)-1);
	TreeView_SetTextColor(m_hWnd, bDark ? CTheme::clrText : (COLORREF)-1);
	TreeView_SetLineColor(m_hWnd, bDark ? CTheme::clrBorder : CLR_DEFAULT);
	InvalidateRect(m_hWnd, nullptr, TRUE);
}

// the line as the item text: trimmed, tabs as spaces, not longer than maxLen
static wstring lineText(CSciWrapper& sci, int line, size_t maxLen = 200) {
	string s = sci.GetTextRange(sci.GetLineIndentPosition(line), sci.GetLineEndPosition(line));
	wstring ws = sci.getEncoding() == encAnsi ? a2w(s) : utf2w(s);
	for( auto& ch : ws )
		if( ch == '\t' )
			ch = ' ';
	while( !ws.empty() && ws.back() == ' ' )
		ws.pop_back();
	if( ws.size() > maxLen )
		ws = ws.substr(0, maxLen) + L"\x2026"; // ...
	return ws;
}

// the attributes of the styles of the editor, read once per Fill()
struct CStyleAttrs {
	struct CAttr { bool bInit; COLORREF fore, back; bool bBack; int font; } attrs[256]{};
	COLORREF backDef;
	explicit CStyleAttrs(CSciWrapper& sci) : backDef((COLORREF)sci.Call(SCI_STYLEGETBACK, STYLE_DEFAULT)) {}
	const CAttr& get(CSciWrapper& sci, int style) {
		CAttr& a = attrs[style & 0xFF];
		if( !a.bInit ) {
			a.bInit = true;
			a.fore = (COLORREF)sci.Call(SCI_STYLEGETFORE, style);
			a.back = (COLORREF)sci.Call(SCI_STYLEGETBACK, style);
			a.bBack = a.back != backDef; // the background of the tree, else: JS/CSS in HTML, {{...}}
			a.font = (sci.Call(SCI_STYLEGETBOLD, style) ? 1 : 0) | (sci.Call(SCI_STYLEGETITALIC, style) ? 2 : 0);
		}
		return a;
	}
};

// appends the styles of a line as runs of its item text, which is size chars (trimmed, a long line is cut)
static void lineRuns(CSciWrapper& sci, int line, int size, CStyleAttrs& attrs, vector<COutline::CRun>& runs) {
	int a = sci.GetLineIndentPosition(line), b = sci.GetLineEndPosition(line);
	vector<COutline::CRun> v;
	if( a < b ) {
		string cells(2 * (b - a) + 2, '\0'); // char, style
		Sci_TextRange tr;
		tr.chrg.cpMin = a;
		tr.chrg.cpMax = b;
		tr.lpstrText = &cells[0];
		sci.Call(SCI_GETSTYLEDTEXT, 0, (intptr_t)&tr);
		bool bAnsi = sci.getEncoding() == encAnsi;
		for( int i = 0; i < b - a; ) {
			int style = (unsigned char)cells[2 * i + 1];
			string s;
			for( ; i < b - a && (unsigned char)cells[2 * i + 1] == style; i++ )
				s += cells[2 * i];
			int len = (int)(bAnsi ? a2w(s) : utf2w(s)).size(); // tabs are spaces in the item: the same length
			const auto& attr = attrs.get(sci, style);
			v.push_back({len, attr.fore, attr.back, attr.bBack, attr.font});
		}
	}
	int total = 0;
	for( auto& run : v ) {
		int len = min(run.len, size - total);
		if( len <= 0 )
			break;
		run.len = len;
		runs.push_back(run);
		total += len;
	}
	if( total < size ) // "..." of a long line, "(12)" of an empty header
		runs.push_back({size - total, (COLORREF)-1, 0, false, 0});
}

static bool isBlank(CSciWrapper& sci, int line) {
	return sci.GetLineIndentPosition(line) == sci.GetLineEndPosition(line);
}

// "{", "};", ")" ...
static bool isBracesOnly(const wstring& ws) {
	return ws.find_first_not_of(L"{}()[];, ") == wstring::npos;
}

// a template tag at the start: "{{- end}}" -> "end}}", "{% endif %}" -> "endif %}", "{{/if}}" -> "/if}}"; else ""
static wstring templateTag(const wstring& ws) {
	static const wchar_t* tags[] = { L"{{", L"{%", L"<%", L"<?" }; // go/handlebars, jinja/twig/liquid, asp/erb/jsp, php
	for( auto sz : tags )
		if( ws.compare(0, 2, sz) == 0 ) {
			size_t i = ws.find_first_not_of(L"-~=# ", 2); // "{{-" trim, "{{~", "<%=", "<%#"
			if( i != wstring::npos && sz[1] == '?' && _wcsnicmp(ws.c_str() + i, L"php", 3) == 0 )
				i = ws.find_first_not_of(L' ', i + 3);
			return i == wstring::npos ? wstring() : ws.substr(i);
		}
	return wstring();
}

// the last line of a fold, which only closes it: "}", "} // foo", "});", "</div>", "*/", "/>", "end;", "{{- end}}"
static bool isClosing(CSciWrapper& sci, int line, int lvl, const wstring& ws) {
	if( isBracesOnly(ws) )
		return true;
	if( line + 1 >= sci.GetLineCount() || (sci.GetFoldLevel(line + 1) & SC_FOLDLEVELNUMBERMASK) >= lvl )
		return false; // not the end of the fold
	auto starts = [&ws](const wchar_t* sz) { return ws.compare(0, wcslen(sz), sz) == 0; };
	if( ws[0] == '}' || ws[0] == ')' || ws[0] == ']' || starts(L"</") || starts(L"*/") || starts(L"/>") || starts(L"-->") )
		return true;
	if( ws.size() >= 3 && _wcsnicmp(ws.c_str(), L"end", 3) == 0 && (ws.size() == 3 || !(iswalnum(ws[3]) || ws[3] == '_')) )
		return true;
	wstring tag = templateTag(ws);
	return (!tag.empty() && tag[0] == '/') // {{/if}}
		|| _wcsnicmp(tag.c_str(), L"end", 3) == 0; // {{end}}, {% endif %}, <% end %>, <?php endif; ?>
}

// a header with "{" alone gets the previous line: "void foo() {"
static bool isBraceHeader(const wstring& ws) {
	return !ws.empty() && (ws[0] == '{' || ws[0] == '(' || ws[0] == '[') && ws.size() <= 2;
}

void COutline::Fill(CSciWrapper& sci) {
	saveCollapsed();
	m_bFilling = true;
	::SendMessage(m_hWnd, WM_SETREDRAW, FALSE, 0);
	deleteAll();
	m_lineToItem.clear();
	m_headers.clear();
	m_styled.clear();
	m_hSci = sci.hWnd();

	sci.ColorizeTo(-1); // the fold levels are set by the lexer
	CStyleAttrs attrs(sci);
	const set<wstring>& collapsed = m_collapsed[m_hSci];
	struct CParent { int level; HTREEITEM h; wstring path; };
	vector<CParent> parents;
	vector<HTREEITEM> expand; // after the children are inserted
	int cnt = sci.GetLineCount();
	bool bAllLines = cnt <= maxAllLines; // a big file: only the headers
	for( int line = 0; line < cnt; line++ ) {
		int lvl = sci.GetFoldLevel(line);
		bool bHeader = (lvl & SC_FOLDLEVELHEADERFLAG) != 0;
		if( !bHeader && (!bAllLines || isBlank(sci, line)) )
			continue;
		lvl &= SC_FOLDLEVELNUMBERMASK;

		int lineItem = line; // the item goes there
		wstring ws;
		size_t lenPrev = 0; // "void foo()" of "void foo() {"
		if( bHeader ) {
			ws = lineText(sci, line);
			if( isBraceHeader(ws) ) {
				int prev = line - 1;
				while( prev >= 0 && isBlank(sci, prev) )
					prev--;
				if( prev >= 0 && !sci.IsFoldHeader(prev) ) {
					wstring wsPrev = lineText(sci, prev);
					lenPrev = wsPrev.size();
					ws = wsPrev + L" " + ws;
					lineItem = prev;
				}
			}
			if( ws.empty() )
				ws = L"(" + to_wstring(line + 1) + L")";
		} else {
			ws = lineText(sci, line);
			if( !m_bShowClosing && isClosing(sci, line, lvl, ws) )
				continue;
			int next = line + 1; // "void foo()" before "{" alone: it is in the item of "{"
			while( next < cnt && isBlank(sci, next) )
				next++;
			if( next < cnt && sci.IsFoldHeader(next) && isBraceHeader(lineText(sci, next)) )
				continue;
		}
		while( !parents.empty() && parents.back().level >= lvl )
			parents.pop_back();

		TVINSERTSTRUCTW is{};
		is.hParent = parents.empty() ? TVI_ROOT : parents.back().h;
		is.hInsertAfter = TVI_LAST;
		is.item.mask = TVIF_TEXT | TVIF_PARAM;
		is.item.pszText = (LPWSTR)ws.c_str();
		is.item.lParam = lineItem;
		auto h = (HTREEITEM)::SendMessageW(m_hWnd, TVM_INSERTITEMW, 0, (LPARAM)&is);
		if( !h )
			break;
		m_lineToItem[line] = h;
		if( lineItem != line )
			m_lineToItem[lineItem] = h;
		vector<CRun> runs; // as in the editor
		if( lineItem != line ) {
			lineRuns(sci, lineItem, (int)lenPrev, attrs, runs);
			runs.push_back({1, (COLORREF)-1, 0, false, 0}); // " "
			lineRuns(sci, line, (int)(ws.size() - lenPrev - 1), attrs, runs);
		} else
			lineRuns(sci, line, (int)ws.size(), attrs, runs);
		m_styled[h] = std::move(runs);
		if( !bHeader )
			continue;

		m_headers[h] = line;
		wstring path = (parents.empty() ? wstring() : parents.back().path + L"\n") + ws;
		if( m_bSyncFold ? sci.GetFoldExpanded(line) : !collapsed.count(path) ) // as in the editor, or as it was
			expand.push_back(h);
		parents.push_back({lvl, h, path});
	}
	for( HTREEITEM h : expand ) // the items without children are not expanded by the tree
		TreeView_Expand(m_hWnd, h, TVE_EXPAND);

	::SendMessage(m_hWnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(m_hWnd, nullptr, TRUE);
	m_bFilling = false;
}

// fast: without a selection the tree does not select the next item for every deleted one
void COutline::deleteAll() {
	TreeView_SelectItem(m_hWnd, nullptr);
	TreeView_DeleteAllItems(m_hWnd);
}

void COutline::Clear() {
	saveCollapsed();
	m_bFilling = true;
	::SendMessage(m_hWnd, WM_SETREDRAW, FALSE, 0);
	deleteAll();
	::SendMessage(m_hWnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(m_hWnd, nullptr, TRUE);
	m_bFilling = false;
	m_lineToItem.clear();
	m_headers.clear();
	m_styled.clear();
	m_hSci = nullptr;
}

void COutline::onSciClosed(HWND hSci) {
	if( m_hSci == hSci )
		Clear();
	m_collapsed.erase(hSci);
}

wstring COutline::ItemText(HTREEITEM h) {
	wchar_t buf[512]{};
	TVITEMW tvi{};
	tvi.hItem = h;
	tvi.mask = TVIF_TEXT;
	tvi.pszText = buf;
	tvi.cchTextMax = 512;
	::SendMessageW(m_hWnd, TVM_GETITEMW, 0, (LPARAM)&tvi);
	return buf;
}

wstring COutline::ItemPath(HTREEITEM h) {
	wstring path = ItemText(h);
	while( (h = TreeView_GetParent(m_hWnd, h)) != nullptr )
		path = ItemText(h) + L"\n" + path;
	return path;
}

void COutline::saveCollapsed() {
	if( m_bSyncFold || !m_hSci )
		return;
	set<wstring>& collapsed = m_collapsed[m_hSci];
	collapsed.clear();
	for( auto& it : m_headers )
		if( TreeView_GetChild(m_hWnd, it.first) && !(TreeView_GetItemState(m_hWnd, it.first, TVIS_EXPANDED) & TVIS_EXPANDED) )
			collapsed.insert(ItemPath(it.first));
}

void COutline::expandChildren(HTREEITEM h, bool bExpand) {
	for( HTREEITEM c = TreeView_GetChild(m_hWnd, h); c; c = TreeView_GetNextSibling(m_hWnd, c) )
		if( TreeView_GetChild(m_hWnd, c) ) {
			expandChildren(c, bExpand);
			TreeView_Expand(m_hWnd, c, bExpand ? TVE_EXPAND : TVE_COLLAPSE);
		}
}

void COutline::ExpandAll(bool bExpand) {
	m_bFilling = true;
	::SendMessage(m_hWnd, WM_SETREDRAW, FALSE, 0);
	for( auto& it : m_headers )
		if( TreeView_GetChild(m_hWnd, it.first) )
			TreeView_Expand(m_hWnd, it.first, bExpand ? TVE_EXPAND : TVE_COLLAPSE);
	::SendMessage(m_hWnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(m_hWnd, nullptr, TRUE);
	m_bFilling = false;
	if( HTREEITEM h = TreeView_GetSelection(m_hWnd) )
		TreeView_EnsureVisible(m_hWnd, h);
}

void COutline::SelectLine(CSciWrapper& sci, int line) {
	if( sci.hWnd() != m_hSci )
		return;
	HTREEITEM h = nullptr;
	for( int i = 0; line >= 0 && i < 1000; i++ ) { // up the fold parents
		auto it = m_lineToItem.find(line);
		if( it != m_lineToItem.end() ) {
			h = it->second;
			break;
		}
		int lineNext = line + 1; // the line before "{" alone
		if( lineNext < sci.GetLineCount() && sci.IsFoldHeader(lineNext) ) {
			it = m_lineToItem.find(lineNext);
			if( it != m_lineToItem.end() && GetLine(it->second) == line ) {
				h = it->second;
				break;
			}
		}
		line = sci.GetFoldParent(line);
	}
	for( HTREEITEM p = h ? TreeView_GetParent(m_hWnd, h) : nullptr; p; p = TreeView_GetParent(m_hWnd, p) )
		if( !(TreeView_GetItemState(m_hWnd, p, TVIS_EXPANDED) & TVIS_EXPANDED) )
			h = p; // in a collapsed item: it is selected, it is not expanded
	if( h == TreeView_GetSelection(m_hWnd) )
		return;
	m_bFilling = true; // not a navigation by the user
	if( h )
		TreeView_SelectItem(m_hWnd, h); // expands the parents and scrolls to it
	else
		TreeView_SelectItem(m_hWnd, nullptr);
	m_bFilling = false;
}

int COutline::GetLine(HTREEITEM h) {
	if( !h )
		return -1;
	TVITEMW tvi{};
	tvi.hItem = h;
	tvi.mask = TVIF_PARAM;
	if( !::SendMessageW(m_hWnd, TVM_GETITEMW, 0, (LPARAM)&tvi) )
		return -1;
	return (int)tvi.lParam;
}

void COutline::SyncFold(CSciWrapper& sci) {
	if( !m_bSyncFold || sci.hWnd() != m_hSci )
		return;
	bool bChanged = false;
	m_bFilling = true; // no TVN_ITEMEXPANDED back to the editor
	for( auto& it : m_headers ) {
		HTREEITEM h = it.first;
		int line = it.second;
		if( !TreeView_GetChild(m_hWnd, h) || !sci.IsFoldHeader(line) )
			continue; // nothing to expand, or the file is changed and the tree is not rebuilt yet
		bool bExpanded = sci.GetFoldExpanded(line);
		if( bExpanded == ((TreeView_GetItemState(m_hWnd, h, TVIS_EXPANDED) & TVIS_EXPANDED) != 0) )
			continue;
		if( !bChanged )
			::SendMessage(m_hWnd, WM_SETREDRAW, FALSE, 0);
		bChanged = true;
		TreeView_Expand(m_hWnd, h, bExpanded ? TVE_EXPAND : TVE_COLLAPSE);
	}
	m_bFilling = false;
	if( bChanged ) {
		::SendMessage(m_hWnd, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(m_hWnd, nullptr, TRUE);
	}
}

void COutline::onItemExpanded(const NMTREEVIEWW* p, CSciWrapper& sci) {
	if( m_bFilling || sci.hWnd() != m_hSci )
		return;
	if( !m_bSyncFold ) { // only the tree
		if( GetKeyState(VK_SHIFT) < 0 ) {
			m_bFilling = true;
			expandChildren(p->itemNew.hItem, (p->action & TVE_EXPAND) != 0);
			m_bFilling = false;
		}
		return;
	}
	auto it = m_headers.find(p->itemNew.hItem);
	if( it == m_headers.end() || !sci.IsFoldHeader(it->second) )
		return;
	bool bExpand = (p->action & TVE_EXPAND) != 0;
	if( GetKeyState(VK_SHIFT) < 0 ) // as Shift + click in the fold margin: the children too; SyncFold() shows them
		sci.doFoldChild(it->second, false, !bExpand, bExpand);
	else
	if( sci.GetFoldExpanded(it->second) != bExpand )
		sci.ToggleFold(it->second);
}

LRESULT COutline::onCustomDraw(NMTVCUSTOMDRAW* pCD) {
	switch( pCD->nmcd.dwDrawStage ) {
	case CDDS_PREPAINT:
		return m_styled.empty() ? CDRF_DODEFAULT : CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPOSTPAINT: { // over the text drawn by the tree
		auto it = m_styled.find((HTREEITEM)pCD->nmcd.dwItemSpec);
		if( it != m_styled.end() )
			drawStyled(pCD, it->second);
		return CDRF_DODEFAULT;
	}
	case CDDS_ITEMPREPAINT: // also the selected item: as the others, with a border around its text
		return m_styled.count((HTREEITEM)pCD->nmcd.dwItemSpec) ? CDRF_NOTIFYPOSTPAINT : CDRF_DODEFAULT;
	}
	return CDRF_DODEFAULT;
}

// the text of the item by the runs: the colors, the backgrounds and the fonts of the editor;
// the selected item has a border: the highlight color if the tree has the focus, else grey
void COutline::drawStyled(NMTVCUSTOMDRAW* pCD, const vector<CRun>& runs) {
	auto h = (HTREEITEM)pCD->nmcd.dwItemSpec;
	CRect r;
	if( !TreeView_GetItemRect(m_hWnd, h, &r, TRUE) )
		return;
	CRect rc;
	::GetClientRect(m_hWnd, &rc);
	r.right = rc.right; // a bold run is wider than the text of the tree
	COLORREF clrBack = TreeView_GetBkColor(m_hWnd);
	if( clrBack == (COLORREF)-1 )
		clrBack = GetSysColor(COLOR_WINDOW);
	COLORREF clrText = TreeView_GetTextColor(m_hWnd);
	if( clrText == (COLORREF)-1 )
		clrText = GetSysColor(COLOR_WINDOWTEXT);

	HDC hDC = pCD->nmcd.hdc;
	SetBkColor(hDC, clrBack);
	ExtTextOutW(hDC, 0, 0, ETO_OPAQUE, &r, nullptr, 0, nullptr); // the text of the tree is erased
	wstring ws = ItemText(h);
	auto hFont = (HFONT)::SendMessage(m_hWnd, WM_GETFONT, 0, 0);
	HGDIOBJ hOld = SelectObject(hDC, hFont);
	int x = r.left + 2 * GetSystemMetrics(SM_CXBORDER); // as the tree
	int pos = 0;
	for( const CRun& run : runs ) {
		if( pos + run.len > (int)ws.size() )
			break;
		SelectObject(hDC, run.font && m_hFonts[run.font] ? m_hFonts[run.font] : hFont);
		SIZE size{};
		GetTextExtentPoint32W(hDC, ws.c_str() + pos, run.len, &size);
		CRect rr(x, r.top, x + size.cx, r.bottom);
		SetBkColor(hDC, run.bBack ? run.back : clrBack);
		SetTextColor(hDC, run.fore == (COLORREF)-1 ? clrText : run.fore);
		ExtTextOutW(hDC, x, r.top + (r.height() - size.cy) / 2, ETO_OPAQUE, &rr, ws.c_str() + pos, run.len, nullptr);
		x += size.cx;
		pos += run.len;
	}
	SelectObject(hDC, hOld);

	if( pCD->nmcd.uItemState & (CDIS_SELECTED | CDIS_FOCUS) ) {
		CRect rf(r.left, r.top, x + 2 * GetSystemMetrics(SM_CXBORDER), r.bottom);
		COLORREF clr = ::GetFocus() == m_hWnd ? GetSysColor(COLOR_HIGHLIGHT)
				: g_theme.isDark() ? CTheme::clrTextDisabled : GetSysColor(COLOR_BTNSHADOW);
		HBRUSH hbr = CreateSolidBrush(clr);
		FrameRect(hDC, &rf, hbr);
		DeleteObject(hbr);
	}
}
