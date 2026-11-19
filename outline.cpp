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
	LOGFONT lf{}; // the comments
	if( ::GetObject(hFont ? hFont : (HFONT)::GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf) ) {
		lf.lfItalic = TRUE;
		m_hFontItalic = CreateFontIndirect(&lf);
	}
	applyTheme();
}

void COutline::Show(bool bShow) {
	::ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
}

void COutline::Move(int x, int y, int w, int h) {
	::MoveWindow(m_hWnd, x, y, max(w, 0), max(h, 0), TRUE);
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
	wstring ws = sci.getEncoding() == encAnsi ? utf2w(a2utf(s)) : utf2w(s);
	for( auto& ch : ws )
		if( ch == '\t' )
			ch = ' ';
	while( !ws.empty() && ws.back() == ' ' )
		ws.pop_back();
	if( ws.size() > maxLen )
		ws = ws.substr(0, maxLen) + L"\x2026"; // ...
	return ws;
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
	TreeView_DeleteAllItems(m_hWnd);
	m_lineToItem.clear();
	m_headers.clear();
	m_comments.clear();
	m_hSci = sci.hWnd();

	sci.ColorizeTo(-1); // the fold levels are set by the lexer
	set<int> commentStyles; // of the lexer: "Comment", "Doc Comment", "Line Comment" ...
	for( CLexerStyle* pStyle = GetLexerStyles(sci); pStyle && pStyle->szName; pStyle++ )
		if( containsIC(pStyle->szName, "comment") )
			commentStyles.insert(pStyle->id);
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
		if( bHeader ) {
			ws = lineText(sci, line);
			if( isBraceHeader(ws) ) {
				int prev = line - 1;
				while( prev >= 0 && isBlank(sci, prev) )
					prev--;
				if( prev >= 0 && !sci.IsFoldHeader(prev) ) {
					ws = lineText(sci, prev) + L" " + ws;
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
		int style = sci.GetStyleAt(sci.GetLineIndentPosition(lineItem)); // the style of the first char
		if( commentStyles.count(style) )
			m_comments[h] = { (COLORREF)sci.Call(SCI_STYLEGETFORE, style), sci.Call(SCI_STYLEGETITALIC, style) != 0 };
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

void COutline::Clear() {
	saveCollapsed();
	m_bFilling = true;
	TreeView_DeleteAllItems(m_hWnd);
	m_bFilling = false;
	m_lineToItem.clear();
	m_headers.clear();
	m_comments.clear();
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
		return m_comments.empty() ? CDRF_DODEFAULT : CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT: {
		auto it = m_comments.find((HTREEITEM)pCD->nmcd.dwItemSpec);
		if( it == m_comments.end() )
			return CDRF_DODEFAULT;
		bool bSelected = (pCD->nmcd.uItemState & CDIS_SELECTED) && ::GetFocus() == m_hWnd;
		if( !bSelected ) // the highlighted item keeps the highlight text color
			pCD->clrText = it->second.fore;
		if( it->second.bItalic && m_hFontItalic ) {
			SelectObject(pCD->nmcd.hdc, m_hFontItalic);
			return CDRF_NEWFONT;
		}
		return CDRF_DODEFAULT;
	}
	}
	return CDRF_DODEFAULT;
}
