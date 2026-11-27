#include "stdafx.h"
#include "table.h"
#include "theme.h"
#include "shared/platform.h"
#include <map>

void CTable::Create(HWND hParent, HFONT hFont) {
	INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_LISTVIEW_CLASSES};
	InitCommonControlsEx(&icc);
	m_hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
			WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
			0, 0, 0, 0, hParent, nullptr, g_hInst, nullptr);
	if( hFont )
		::SendMessage(m_hWnd, WM_SETFONT, (WPARAM)hFont, FALSE);
	LOGFONT lf{}; // the styles of the editor: bold, italic, bold italic
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
	::SendMessage(m_hWnd, CCM_SETUNICODEFORMAT, TRUE, 0); // LVN_GETDISPINFOW
	// full row select: a click anywhere in the row selects it; only the cell is drawn selected (onCustomDraw)
	ListView_SetExtendedListViewStyle(m_hWnd, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP);
	::SendMessage(m_hWnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0); // no focus rectangle of the row
	SetWindowSubclass(m_hWnd, ListProc, 1, (DWORD_PTR)this);
	applyTheme();
}

void CTable::Show(bool bShow) {
	::ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
}

void CTable::Move(int x, int y, int w, int h) {
	::MoveWindow(m_hWnd, x, y, max(w, 0), max(h, 0), TRUE);
}

void CTable::applyTheme() {
	memset(m_attrs, 0, sizeof(m_attrs)); // the editor colors are converted for dark mode
	g_theme.applySci(m_hWnd); // scrollbars + no 3D border in dark mode
	g_theme.applyCtrl(ListView_GetHeader(m_hWnd));
	bool bDark = g_theme.isDark();
	COLORREF clrBack = bDark ? CTheme::clrEditorBack : GetSysColor(COLOR_WINDOW);
	ListView_SetBkColor(m_hWnd, clrBack);
	ListView_SetTextBkColor(m_hWnd, clrBack);
	ListView_SetTextColor(m_hWnd, bDark ? CTheme::clrText : GetSysColor(COLOR_WINDOWTEXT));
	InvalidateRect(m_hWnd, nullptr, TRUE);
}

/////////////////////////////////////////////////////////////////////
// the cells

// "a";b;"c""d";;e -> [a] [b] [c""d] [] [e]: a quoted cell is without its quotes, "" stays in the text
static void splitBuf(const char* s, int len, char sep, vector<CTable::CCell>& cells) {
	cells.clear();
	int i = 0;
	while( true ) {
		if( i < len && s[i] == '"' ) { // quoted: up to the closing quote, the rest up to the separator is ignored
			int start = ++i;
			while( i < len && !(s[i] == '"' && !(i + 1 < len && s[i + 1] == '"')) )
				i += s[i] == '"' ? 2 : 1;
			cells.push_back({start, i, true});
			while( i < len && s[i] != sep )
				i++;
		} else {
			int start = i;
			while( i < len && s[i] != sep )
				i++;
			cells.push_back({start, i, false});
		}
		if( i >= len )
			break;
		i++; // the separator: there is one more cell, also an empty one at the end
	}
}

void CTable::Split(const string& s, char sep, vector<CCell>& cells) {
	splitBuf(s.data(), (int)s.size(), sep, cells);
}

// the lines of the document, without the EOL; f returns false to stop
template<class F> static void forLines(HWND hSci, int maxLines, F f) {
	const char* p = (const char*)::SendMessage(hSci, SCI_GETCHARACTERPOINTER, 0, 0); // contiguous, until the next change
	int len = (int)::SendMessage(hSci, SCI_GETLENGTH, 0, 0);
	if( !p )
		return;
	int start = 0;
	for( int line = 0; line < maxLines && start <= len; line++ ) {
		int end = start;
		while( end < len && p[end] != '\n' && p[end] != '\r' )
			end++;
		if( !f(line, p + start, end - start) || end >= len )
			break;
		end += (p[end] == '\r' && end + 1 < len && p[end + 1] == '\n') ? 2 : 1;
		start = end;
	}
}

// the separator, which gives the same number (> 1) of cells in the most lines; the order wins on a tie
char CTable::DetectSep(CSciWrapper& sci) {
	static const char seps[] = {'\t', ';', ','};
	int score[3]{};
	vector<CCell> cells;
	for( int k = 0; k < 3; k++ ) {
		std::map<int, int> counts; // cells -> lines
		int nonEmpty = 0;
		forLines(sci.hWnd(), INT_MAX, [&](int, const char* s, int len) {
			if( !len )
				return true;
			splitBuf(s, len, seps[k], cells);
			counts[(int)cells.size()]++;
			return ++nonEmpty < 200;
		});
		for( auto& c : counts )
			if( c.first > 1 && c.second > score[k] )
				score[k] = c.second;
	}
	int best = 0;
	for( int k = 1; k < 3; k++ )
		if( score[k] > score[best] )
			best = k;
	return seps[best];
}

const vector<CTable::CCell>& CTable::cells(int line, string* pText) {
	if( line != m_cacheLine ) {
		m_cacheLine = line;
		m_cacheText.clear();
		m_cacheCells.clear();
		int lines = (int)::SendMessage(m_hSci, SCI_GETLINECOUNT, 0, 0);
		if( m_hSci && line >= 0 && line < lines ) {
			int p1 = (int)::SendMessage(m_hSci, SCI_POSITIONFROMLINE, line, 0);
			int p2 = (int)::SendMessage(m_hSci, SCI_GETLINEENDPOSITION, line, 0);
			if( p2 > p1 ) {
				m_cacheText.resize(p2 - p1 + 1);
				Sci_TextRange tr;
				tr.chrg.cpMin = p1;
				tr.chrg.cpMax = p2;
				tr.lpstrText = &m_cacheText[0];
				::SendMessage(m_hSci, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);
				m_cacheText.resize(p2 - p1);
			}
			Split(m_cacheText, m_sep, m_cacheCells);
		}
	}
	if( pText )
		*pText = m_cacheText;
	return m_cacheCells;
}

/////////////////////////////////////////////////////////////////////

void CTable::Clear() {
	m_bFilling = true;
	m_hSci = nullptr;
	m_rows = 0;
	m_cacheLine = -1;
	ListView_SetItemCountEx(m_hWnd, 0, 0);
	while( ListView_DeleteColumn(m_hWnd, 0) )
		;
	m_cols = 0;
	m_bFilling = false;
}

void CTable::onSciClosed(HWND hSci) {
	if( hSci == m_hSci )
		Clear();
}

void CTable::Fill(CSciWrapper& sci) {
	m_bFilling = true;
	bool bNewFile = sci.hWnd() != m_hSci;
	memset(m_attrs, 0, sizeof(m_attrs)); // another file, another lexer; or the Colors are changed
	m_hSci = sci.hWnd();
	m_bAnsi = sci.getEncoding() == encAnsi;
	static const char seps[] = {0, '\t', ';', ','};
	char sep = m_sepMode == sepAuto ? DetectSep(sci) : seps[m_sepMode];
	bool bNewSep = sep != m_sep;
	m_sep = sep;
	m_cacheLine = -1;

	// the rows: every line; the empty line after the last EOL is not a row
	int rows = sci.GetLineCount();
	if( rows > 1 && sci.PositionFromLine(rows - 1) == sci.GetLineEndPosition(rows - 1) )
		rows--;

	int cols = 1;
	vector<CCell> v;
	forLines(m_hSci, min(rows, maxScanLines), [&](int, const char* s, int len) {
		splitBuf(s, len, m_sep, v);
		cols = max(cols, (int)v.size());
		return true;
	});
	cols = min(cols, maxCols);

	::SendMessage(m_hWnd, WM_SETREDRAW, FALSE, 0);
	if( bNewFile || bNewSep || cols != m_cols ) { // the columns again; else the widths, set by the user, are kept
		while( ListView_DeleteColumn(m_hWnd, 0) )
			;
		// the widths by the first rows
		vector<int> widths(cols, g_dpi.scale(40));
		int pad = g_dpi.scale(14);
		for( int row = 0; row < min(rows, 50); row++ ) {
			string sLine;
			const vector<CCell>& rc = cells(row, &sLine);
			for( int c = 0; c < (int)rc.size() && c < cols; c++ ) {
				string s = sLine.substr(rc[c].start, min(rc[c].end - rc[c].start, 100));
				wstring ws = utf2w(m_bAnsi ? a2utf(s) : s);
				int w = (int)::SendMessage(m_hWnd, LVM_GETSTRINGWIDTHW, 0, (LPARAM)ws.c_str()) + pad;
				widths[c] = max(widths[c], min(w, g_dpi.scale(400)));
			}
		}
		LVCOLUMNW col{};
		col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		col.cx = g_dpi.scale(40);
		col.pszText = (LPWSTR)L"#";
		col.iSubItem = 0;
		::SendMessage(m_hWnd, LVM_INSERTCOLUMNW, 0, (LPARAM)&col); // the line numbers
		for( int c = 0; c < cols; c++ ) {
			wstring wsHeader = to_wstring(c + 1);
			col.cx = widths[c];
			col.pszText = (LPWSTR)wsHeader.c_str();
			col.iSubItem = c + 1;
			::SendMessage(m_hWnd, LVM_INSERTCOLUMNW, c + 1, (LPARAM)&col);
		}
		m_cols = cols;
		m_selCol = min(m_selCol, cols - 1);
	}
	m_rows = rows;
	ListView_SetColumnWidth(m_hWnd, 0, lineNoWidth()); // the digits of the last line
	ListView_SetItemCountEx(m_hWnd, rows, LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
	::SendMessage(m_hWnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(m_hWnd, nullptr, TRUE);
	m_bFilling = false;
}

void CTable::invalidateRow(int row) {
	RECT r;
	if( row >= 0 && ListView_GetItemRect(m_hWnd, row, &r, LVIR_BOUNDS) )
		InvalidateRect(m_hWnd, &r, FALSE);
}

void CTable::SelectCell(int line, int offset) {
	if( line < 0 || line >= m_rows )
		return;
	int col = m_selCol;
	if( offset >= 0 ) { // the last cell starting at or before the offset (a quoted one with its quote)
		const vector<CCell>& rc = cells(line);
		col = 0;
		for( int i = 0; i < (int)rc.size(); i++ )
			if( rc[i].start - (rc[i].bQuoted ? 1 : 0) <= offset )
				col = i;
		col = min(col, m_cols - 1);
	}
	if( selRow() != line || ListView_GetNextItem(m_hWnd, -1, LVNI_SELECTED) != line ) {
		m_bFilling = true;
		ListView_SetItemState(m_hWnd, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
		ListView_SetItemState(m_hWnd, line, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		ListView_SetSelectionMark(m_hWnd, line);
		ListView_EnsureVisible(m_hWnd, line, FALSE);
		m_bFilling = false;
	}
	SetSelCol(col);
}

void CTable::SetSelCol(int col) {
	col = max(0, min(col, m_cols - 1));
	int row = selRow();
	if( col != m_selCol ) {
		m_selCol = col;
		invalidateRow(row);
		invalidateHeader();
	}
	// horizontally into view
	if( row < 0 )
		row = ListView_GetTopIndex(m_hWnd);
	RECT r, rc;
	if( m_rows <= 0 || !ListView_GetSubItemRect(m_hWnd, row, col + 1, LVIR_BOUNDS, &r) )
		return;
	::GetClientRect(m_hWnd, &rc);
	int dx = 0;
	vector<int> order = dataOrder();
	if( !order.empty() && order[0] == col ) // the first cell: the view at its start, with the line numbers
		dx = -::GetScrollPos(m_hWnd, SB_HORZ);
	else
	if( r.left < rc.left )
		dx = r.left - rc.left;
	else
	if( r.right > rc.right )
		dx = min(r.right - rc.right, r.left - rc.left); // a cell wider than the view: its left edge
	if( dx )
		ListView_Scroll(m_hWnd, dx, 0);
}

vector<int> CTable::dataOrder() {
	vector<int> all(m_cols + 1), order; // the list columns by the display order -> the data columns
	if( m_cols <= 0 )
		return order;
	if( !ListView_GetColumnOrderArray(m_hWnd, m_cols + 1, all.data()) )
		for( int i = 0; i <= m_cols; i++ )
			all[i] = i;
	for( int sub : all )
		if( sub > 0 )
			order.push_back(sub - 1);
	return order;
}

void CTable::MoveSelCol(int delta) {
	if( m_cols <= 0 )
		return;
	vector<int> order = dataOrder();
	int pos = 0;
	for( int i = 0; i < (int)order.size(); i++ )
		if( order[i] == m_selCol )
			pos = i;
	pos = max(0, min(pos + delta, (int)order.size() - 1));
	SetSelCol(order[pos]);
}

static COLORREF mixColor(COLORREF a, COLORREF b, int percentA) {
	auto mix = [percentA](int x, int y) { return (x * percentA + y * (100 - percentA)) / 100; };
	return RGB(mix(GetRValue(a), GetRValue(b)), mix(GetGValue(a), GetGValue(b)), mix(GetBValue(a), GetBValue(b)));
}

LRESULT CTable::onCustomDraw(NMLVCUSTOMDRAW* pCD) {
	switch( pCD->nmcd.dwDrawStage ) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT: // no row highlight, no row focus rectangle
		pCD->nmcd.uItemState &= ~(CDIS_SELECTED | CDIS_FOCUS);
		return CDRF_NOTIFYSUBITEMDRAW;
	case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
		pCD->nmcd.uItemState &= ~(CDIS_SELECTED | CDIS_FOCUS);
		int row = (int)pCD->nmcd.dwItemSpec;
		bool bCurRow = (ListView_GetItemState(m_hWnd, row, LVIS_SELECTED) & LVIS_SELECTED) != 0;
		if( pCD->iSubItem == 0 ) {
			drawLineNo(pCD->nmcd.hdc, row, bCurRow);
			return CDRF_SKIPDEFAULT;
		}
		int col = pCD->iSubItem - 1; // the data column
		bool bSel = bCurRow && col == m_selCol;
		COLORREF back, foreSel = (COLORREF)-1; // -1: the text in the colors of the editor
		if( !bSel ) {
			back = g_theme.paneBack();
			if( bCurRow ) // the other cells of the current row: the selection color at 10% over the background
				back = g_theme.rowBack(back);
		} else {
			back = g_theme.selBack(hasFocus()); // not focused: the cell is still seen
			if( hasFocus() ) // a strong background: one readable color
				foreSel = g_theme.selText();
		}
		drawCell(pCD->nmcd.hdc, row, col, back, foreSel);
		return CDRF_SKIPDEFAULT;
	}
	}
	return CDRF_DODEFAULT;
}

const CTable::CAttr& CTable::attr(int style) {
	CAttr& a = m_attrs[style & 0xFF];
	if( !a.bInit ) {
		a.bInit = true;
		COLORREF backDef = (COLORREF)::SendMessage(m_hSci, SCI_STYLEGETBACK, STYLE_DEFAULT, 0);
		a.fore = (COLORREF)::SendMessage(m_hSci, SCI_STYLEGETFORE, style, 0);
		a.back = (COLORREF)::SendMessage(m_hSci, SCI_STYLEGETBACK, style, 0);
		a.bBack = a.back != backDef; // else the background of the table: the selection, the current row
		a.font = (::SendMessage(m_hSci, SCI_STYLEGETBOLD, style, 0) ? 1 : 0) | (::SendMessage(m_hSci, SCI_STYLEGETITALIC, style, 0) ? 2 : 0);
	}
	return a;
}

void CTable::drawCell(HDC hDC, int row, int col, COLORREF back, COLORREF foreSel) {
	RECT r;
	if( !ListView_GetSubItemRect(m_hWnd, row, col + 1, LVIR_BOUNDS, &r) )
		return;
	SetBkColor(hDC, back);
	ExtTextOutW(hDC, 0, 0, ETO_OPAQUE, &r, nullptr, 0, nullptr);

	int lines = (int)::SendMessage(m_hSci, SCI_GETLINECOUNT, 0, 0);
	if( !m_hSci || row >= lines )
		return;
	const vector<CCell>& rc = cells(row);
	if( col >= (int)rc.size() || rc[col].end <= rc[col].start )
		return;
	int lineStart = (int)::SendMessage(m_hSci, SCI_POSITIONFROMLINE, row, 0);
	int p1 = lineStart + rc[col].start;
	int p2 = lineStart + min(rc[col].end, rc[col].start + 1000); // a long cell: only its start is seen
	// the lexer styles only the shown part of the editor: up to this cell
	int lineEnd = (int)::SendMessage(m_hSci, SCI_GETLINEENDPOSITION, row, 0);
	if( (int)::SendMessage(m_hSci, SCI_GETENDSTYLED, 0, 0) < lineEnd )
		::SendMessage(m_hSci, SCI_COLOURISE, ::SendMessage(m_hSci, SCI_GETENDSTYLED, 0, 0), lineEnd);

	string styled(2 * (p2 - p1) + 2, '\0'); // char, style
	Sci_TextRange tr;
	tr.chrg.cpMin = p1;
	tr.chrg.cpMax = p2;
	tr.lpstrText = &styled[0];
	::SendMessage(m_hSci, SCI_GETSTYLEDTEXT, 0, (LPARAM)&tr);

	int saved = SaveDC(hDC);
	IntersectClipRect(hDC, r.left, r.top, r.right, r.bottom); // the text is clipped by the cell
	auto hFont = (HFONT)::SendMessage(m_hWnd, WM_GETFONT, 0, 0);
	HGDIOBJ hOld = SelectObject(hDC, hFont);
	int x = r.left + g_dpi.scale(6); // as the list view
	int n = p2 - p1;
	for( int i = 0; i < n && x < r.right; ) {
		int style = (unsigned char)styled[2 * i + 1];
		string s;
		for( ; i < n && (unsigned char)styled[2 * i + 1] == style; i++ )
			s += styled[2 * i];
		if( rc[col].bQuoted )
			replaceStr(s, "\"\"", "\"");
		wstring ws = utf2w(m_bAnsi ? a2utf(s) : s);
		for( auto& ch : ws )
			if( ch < ' ' )
				ch = ' ';
		const CAttr& a = attr(style);
		SelectObject(hDC, a.font && m_hFonts[a.font] ? m_hFonts[a.font] : hFont);
		SIZE size{};
		GetTextExtentPoint32W(hDC, ws.c_str(), (int)ws.size(), &size);
		RECT rr = {x, r.top, x + size.cx, r.bottom};
		bool bBack = a.bBack && foreSel == (COLORREF)-1;
		SetBkColor(hDC, bBack ? a.back : back);
		SetTextColor(hDC, foreSel != (COLORREF)-1 ? foreSel : a.fore);
		ExtTextOutW(hDC, x, r.top + (r.bottom - r.top - size.cy) / 2, ETO_OPAQUE | ETO_CLIPPED, &rr, ws.c_str(), (UINT)ws.size(), nullptr);
		x += size.cx;
	}
	SelectObject(hDC, hOld);
	RestoreDC(hDC, saved);
}

int CTable::lineNoWidth() {
	wstring ws(to_wstring(max(m_rows, 1)).size() + 1, L'9'); // + a digit of room
	return (int)::SendMessage(m_hWnd, LVM_GETSTRINGWIDTHW, 0, (LPARAM)ws.c_str()) + g_dpi.scale(14);
}

// as the line numbers of the editor: their colors; the current line in bold
void CTable::drawLineNo(HDC hDC, int row, bool bCurRow) {
	RECT r, rl;
	if( !ListView_GetSubItemRect(m_hWnd, row, 0, LVIR_BOUNDS, &r) ) // the bounds of the column 0 are the whole row
		return;
	if( ListView_GetSubItemRect(m_hWnd, row, 0, LVIR_LABEL, &rl) )
		r.right = rl.right;
	bool bDark = g_theme.isDark();
	COLORREF fore = bDark ? CTheme::clrTextDisabled : GetSysColor(COLOR_GRAYTEXT);
	COLORREF back = bDark ? CTheme::clrMargin : GetSysColor(COLOR_BTNFACE);
	if( m_hSci ) {
		const CAttr& a = attr(STYLE_LINENUMBER);
		fore = a.fore;
		back = a.back;
	}
	if( bCurRow )
		back = g_theme.rowBack(back);
	SetBkColor(hDC, back);
	ExtTextOutW(hDC, 0, 0, ETO_OPAQUE, &r, nullptr, 0, nullptr);
	wstring ws = to_wstring(row + 1);
	auto hFont = (HFONT)::SendMessage(m_hWnd, WM_GETFONT, 0, 0);
	HGDIOBJ hOld = SelectObject(hDC, bCurRow && m_hFonts[1] ? m_hFonts[1] : hFont);
	SetTextColor(hDC, fore);
	SetBkMode(hDC, TRANSPARENT);
	RECT rt = r;
	rt.right -= g_dpi.scale(6);
	DrawTextW(hDC, ws.c_str(), (int)ws.size(), &rt, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	SetBkMode(hDC, OPAQUE);
	SelectObject(hDC, hOld);
}

LRESULT CTable::onHeaderDraw(NMCUSTOMDRAW* pCD) {
	HWND hHeader = pCD->hdr.hwndFrom;
	bool bDark = g_theme.isDark();
	COLORREF clrBack = bDark ? CTheme::clrCtrl : GetSysColor(COLOR_BTNFACE);
	COLORREF clrLine = bDark ? CTheme::clrBorder : GetSysColor(COLOR_3DSHADOW);
	switch( pCD->dwDrawStage ) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
	case CDDS_ITEMPREPAINT: {
		HDC hdc = pCD->hdc;
		RECT r = pCD->rc;
		int col = (int)pCD->dwItemSpec;
		bool bCur = col > 0 && col - 1 == m_selCol && selRow() >= 0; // col: the list column
		COLORREF back = bCur ? mixColor(g_theme.selBack(true), clrBack, hasFocus() ? 45 : 25) : clrBack;
		if( pCD->uItemState & CDIS_SELECTED ) // pressed
			back = mixColor(bDark ? RGB(0xFF,0xFF,0xFF) : RGB(0,0,0), back, 10);
		HBRUSH hbr = CreateSolidBrush(back);
		FillRect(hdc, &r, hbr);
		DeleteObject(hbr);
		hbr = CreateSolidBrush(clrLine); // the right and the bottom border
		RECT rl = {r.right - 1, r.top, r.right, r.bottom};
		FillRect(hdc, &rl, hbr);
		RECT rb = {r.left, r.bottom - 1, r.right, r.bottom};
		FillRect(hdc, &rb, hbr);
		DeleteObject(hbr);

		wchar_t sz[32]{};
		HDITEMW hdi{};
		hdi.mask = HDI_TEXT;
		hdi.pszText = sz;
		hdi.cchTextMax = 32;
		::SendMessage(hHeader, HDM_GETITEMW, col, (LPARAM)&hdi);
		HFONT hOld = (HFONT)SelectObject(hdc, (HFONT)::SendMessage(hHeader, WM_GETFONT, 0, 0));
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, bDark ? CTheme::clrText : GetSysColor(COLOR_BTNTEXT));
		DrawTextW(hdc, sz, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
		SelectObject(hdc, hOld);
		return CDRF_SKIPDEFAULT;
	}
	case CDDS_POSTPAINT: { // right of the last column
		RECT rc, r{};
		::GetClientRect(hHeader, &rc);
		int cnt = Header_GetItemCount(hHeader);
		if( cnt > 0 )
			Header_GetItemRect(hHeader, Header_OrderToIndex(hHeader, cnt - 1), &r);
		rc.left = r.right;
		if( rc.left < rc.right ) {
			HBRUSH hbr = CreateSolidBrush(clrBack);
			FillRect(pCD->hdc, &rc, hbr);
			DeleteObject(hbr);
			hbr = CreateSolidBrush(clrLine);
			RECT rb = {rc.left, rc.bottom - 1, rc.right, rc.bottom};
			FillRect(pCD->hdc, &rb, hbr);
			DeleteObject(hbr);
		}
		return CDRF_DODEFAULT;
	}
	}
	return CDRF_DODEFAULT;
}

// Left/Right: the selected cell, not the horizontal scroll; a click selects the cell under the mouse
LRESULT CALLBACK CTable::ListProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR ref) {
	CTable* p = (CTable*)ref;
	switch( msg ) {
	case WM_KEYDOWN:
		if( wParam == VK_LEFT || wParam == VK_RIGHT ) {
			p->MoveSelCol(wParam == VK_LEFT ? -1 : +1);
			return 0;
		}
		if( wParam == VK_HOME || wParam == VK_END ) {
			bool bEnd = wParam == VK_END;
			if( ::GetKeyState(VK_CONTROL) & 0x8000 ) { // Ctrl: the first/last line, the column is kept
				if( p->m_rows > 0 )
					p->SelectCell(bEnd ? p->m_rows - 1 : 0);
			} else { // the first/last cell of the line
				int row = p->selRow();
				if( row >= 0 )
					p->SetSelCol(bEnd ? (int)p->cells(row).size() - 1 : 0);
			}
			return 0;
		}
		break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDBLCLK: {
		LVHITTESTINFO hti{};
		hti.pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		if( ListView_SubItemHitTest(hWnd, &hti) >= 0 && hti.iItem >= 0 && hti.iSubItem > 0 && hti.iSubItem - 1 != p->m_selCol ) {
			p->invalidateRow(p->selRow());
			p->m_selCol = hti.iSubItem - 1;
			p->invalidateRow(hti.iItem);
			p->invalidateHeader();
		}
		break;
	}
	case WM_NOTIFY: // the header sends its NM_CUSTOMDRAW to the list view
		if( ((NMHDR*)lParam)->hwndFrom == ListView_GetHeader(hWnd) && ((NMHDR*)lParam)->code == NM_CUSTOMDRAW )
			return p->onHeaderDraw((NMCUSTOMDRAW*)lParam);
		break;
	case WM_SETFOCUS:
	case WM_KILLFOCUS: { // the color of the selected cell
		LRESULT lr = DefSubclassProc(hWnd, msg, wParam, lParam);
		p->invalidateRow(p->selRow());
		p->invalidateHeader();
		return lr;
	}
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

bool CTable::GetCellRange(CSciWrapper& sci, int row, int col, int& p1, int& p2) {
	if( sci.hWnd() != m_hSci || row < 0 || row >= sci.GetLineCount() || col < 0 )
		return false;
	m_cacheLine = -1; // the text now: the table can be older than an edit
	const vector<CCell>& rc = cells(row);
	int lineStart = sci.PositionFromLine(row);
	if( col >= (int)rc.size() ) { // no such cell in this row: the end of the line
		p1 = p2 = sci.GetLineEndPosition(row);
		return true;
	}
	p1 = lineStart + rc[col].start;
	p2 = lineStart + rc[col].end;
	return true;
}

void CTable::onGetDispInfo(NMLVDISPINFOW* pDI) {
	LVITEMW& item = pDI->item;
	if( !(item.mask & LVIF_TEXT) || !item.pszText || item.cchTextMax <= 0 )
		return;
	item.pszText[0] = 0;
	if( item.iSubItem == 0 ) { // the line number
		wstring ws = to_wstring(item.iItem + 1);
		wcsncpy_s(item.pszText, item.cchTextMax, ws.c_str(), _TRUNCATE);
		return;
	}
	string sLine;
	const vector<CCell>& rc = cells(item.iItem, &sLine);
	int col = item.iSubItem - 1;
	if( col < 0 || col >= (int)rc.size() )
		return;
	const CCell& cell = rc[col];
	string s = sLine.substr(cell.start, min(cell.end - cell.start, item.cchTextMax * 4));
	if( cell.bQuoted )
		replaceStr(s, "\"\"", "\"");
	wstring ws = utf2w(m_bAnsi ? a2utf(s) : s);
	for( auto& ch : ws )
		if( ch < ' ' )
			ch = ' ';
	size_t n = min(ws.size(), (size_t)item.cchTextMax - 1);
	wmemcpy(item.pszText, ws.c_str(), n);
	item.pszText[n] = 0;
}
