#include "stdafx.h"
#include "resource.h"
#include "apeEditor.h"
#include "options.h"
#include "lexerStyles.h"
#include "lang.h"
#include "theme.h"

// CEditor: table: the lines of the current file split into cells, a list view under the editor

void CEditor::ShowTable( bool bShow ) {
	m_bTableAuto = false; // the user decided: kept also for other files
	m_table.m_bVisible = bShow;
	UpdateTableShown();
	FillTable();
}

bool CEditor::isTableFile( const string& sFile ) {
	string sExt = ExtractFileExt( sFile );
	return equalsToIC(sExt, "csv") || equalsToIC(sExt, "tsv") || equalsToIC(sExt, "tab");
}

void CEditor::AutoShowTable( const CTabPage& page ) {
	if( isTableFile(page.sFile) ) {
		if( !m_table.m_bVisible ) {
			m_table.m_bVisible = true;
			m_bTableAuto = true;
			UpdateTableShown();
		}
	} else
	if( m_bTableAuto && m_table.m_bVisible ) {
		m_table.m_bVisible = false;
		m_bTableAuto = false;
		UpdateTableShown();
	}
}

void CEditor::UpdateTableShown() {
	bool bShow = TableShown();
	if( bShow==m_table.isShown() )
		return;
	if( !bShow ) {
		if( m_table.hasFocus() ) {
			CTabPage* pPage = GetCurPage();
			::SetFocus( pPage ? pPage->sci.hWnd() : m_hWnd );
		}
		KillTimer( ID_TABLE );
		m_table.Clear();
	}
	m_table.Show( bShow );
	CRect r = GetClientRect();
	onSize( r.width(), r.height() );
}

int CEditor::TableH() {
	if( !TableShown() )
		return 0;
	CRect r = GetClientRect();
	int hMin = g_dpi.scale(50);
	int hMax = r.height()-TOOLBAR_H-TAB_H-STATUSBAR_H-ResultsH()-hMin; // the editor keeps some lines
	return max( min(m_table.m_height, hMax), hMin );
}

CRect CEditor::TableSplitterRect() {
	CRect r = GetClientRect();
	int y = r.height()-STATUSBAR_H-ResultsH()-TableH();
	return CRect( 0, y, r.width()-OutlineW(), y+CTable::SplitterH() ); // the outline is at the right
}

void CEditor::FillTable() {
	KillTimer( ID_TABLE );
	if( !TableShown() )
		return;
	CTabPage* pPage = GetCurPage();
	if( !pPage ) {
		m_table.Clear();
		return;
	}
	m_table.Fill( pPage->sci );
	int line = pPage->sci.GetCurrentLine();
	m_table.SelectCell( line, pPage->sci.GetCurrentPos()-pPage->sci.PositionFromLine(line) );
}

LRESULT CEditor::onNotificationTable( NMHDR* pNMHdr ) {
	HWND hList = m_table.hWnd();
	switch( pNMHdr->code ) {
	case LVN_GETDISPINFOW:
		m_table.onGetDispInfo( (NMLVDISPINFOW*)pNMHdr );
		break;
	case NM_SETFOCUS: // the editor tabs and the results tabs are grey
		SetActiveTabs( viewTable );
		break;
	case NM_CUSTOMDRAW: // only the selected cell is highlighted, not the row
		return m_table.onCustomDraw( (NMLVCUSTOMDRAW*)pNMHdr );
	case NM_CLICK: // Ctrl+click: the text of the cell is selected in the editor, the focus stays in the table
	case NM_DBLCLK: // double-click: the same, the focus goes to the editor
		{
			bool bDbl = pNMHdr->code==NM_DBLCLK;
			if( !bDbl && !(::GetKeyState(VK_CONTROL) & 0x8000) )
				break;
			LVHITTESTINFO hti{};
			hti.pt = ((NMITEMACTIVATE*)pNMHdr)->ptAction;
			if( ListView_SubItemHitTest(hList, &hti)<0 || hti.iItem<0 )
				break;
			// Ctrl+click toggles the selection: the clicked row stays selected
			ListView_SetItemState( hList, hti.iItem, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED );
			onTableGoto( hti.iItem, hti.iSubItem-1, bDbl ); // iSubItem: the list column, also if the columns are reordered; 0: the line number -> the line
		}
		return TRUE;
	case NM_RETURN: // the selected cell
		onTableGoto( m_table.selRow(), m_table.selCol(), true );
		return TRUE;
	case LVN_KEYDOWN:
		if( ((NMLVKEYDOWN*)pNMHdr)->wVKey==VK_ESCAPE ) { // close the table, the focus goes to the editor
			ShowTable( false );
			return TRUE;
		}
		break;
	}
	return 0;
}

void CEditor::onTableGoto( int row, int col, bool bFocus ) {
	CTabPage* pPage = GetCurPage();
	if( !pPage || pPage->sci.hWnd()!=m_table.hSci() || row<0 || row>=pPage->sci.GetLineCount() )
		return;
	CSciWrapper& sci = pPage->sci;
	int p1, p2;
	if( col<0 ) { // the line number: the whole line, the caret at its start (no horizontal scroll)
		p1 = sci.GetLineEndPosition( row );
		p2 = sci.PositionFromLine( row );
	} else
	if( !m_table.GetCellRange(sci, row, col, p1, p2) )
		return;
	sci.EnsureVisible( p1, p2 ); // unfold
	int vis = sci.Call( SCI_VISIBLEFROMDOCLINE, row );
	int first = sci.GetFirstVisibleLine();
	if( vis<first || vis>=first+sci.LinesOnScreen() ) // not at the edge of the editor: a third from the top
		sci.SetFirstVisibleLine( max(vis-sci.LinesOnScreen()/3, 0) );
	sci.SetSel( p1, p2 );
	sci.Call( SCI_SCROLLRANGE, max(p1,p2), min(p1,p2) ); // a cell far at the right: its start is seen first
	::SetFocus( bFocus ? sci.hWnd() : m_table.hWnd() );
}

void CEditor::onPopupTable( int x, int y ) {
	HWND hList = m_table.hWnd();
	if( x==-1 && y==-1 ) { // from the keyboard: under the focused row
		CRect r( 0, 0, 0, 0 );
		int i = ListView_GetNextItem( hList, -1, LVNI_FOCUSED );
		if( i>=0 )
			ListView_GetItemRect( hList, i, &r, LVIR_BOUNDS );
		POINT pt = { r.left, r.bottom };
		::ClientToScreen( hList, &pt );
		x = pt.x; y = pt.y;
	}
	static const wchar_t* sepNames[] = { L"Tab (\\t)", L"Semicolon (;)", L"Comma (,)" };
	char sep = m_table.sep();
	wstring wsAuto = tr(L"Auto");
	if( m_table.hSci() && m_table.m_sepMode==CTable::sepAuto ) // the detected one
		wsAuto += L": " + tr(sepNames[sep=='\t' ? 0 : sep==';' ? 1 : 2]);
	CMenu popup;
	popup.Create(true);
	popup.Append( ID_TABLE_SEP_AUTO, wsAuto.c_str() );
	popup.Append( ID_TABLE_SEP_TAB, tr(sepNames[0]) );
	popup.Append( ID_TABLE_SEP_SEMICOLON, tr(sepNames[1]) );
	popup.Append( ID_TABLE_SEP_COMMA, tr(sepNames[2]) );
	popup.Check( ID_TABLE_SEP_AUTO+m_table.m_sepMode, true );
	popup.AppendSeparator();
	popup.Append( ID_TABLE_REFRESH, tr(L"Refresh") );
	popup.AppendSeparator();
	popup.Append( ID_VIEW_TABLE, tr(L"Hide") );
	int cmd = popup.TrackPopup( TPM_RETURNCMD, x, y, m_hWnd );
	switch( cmd ) {
	case ID_TABLE_SEP_AUTO: case ID_TABLE_SEP_TAB: case ID_TABLE_SEP_SEMICOLON: case ID_TABLE_SEP_COMMA:
		m_table.m_sepMode = cmd-ID_TABLE_SEP_AUTO;
		FillTable();
		break;
	case ID_TABLE_REFRESH: FillTable(); break;
	case ID_VIEW_TABLE: ShowTable( false ); break;
	}
}
