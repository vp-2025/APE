#include "stdafx.h"
#include "resource.h"
#include "apeEditor.h"
#include "options.h"
#include "lexerStyles.h"
#include "lang.h"
#include "theme.h"

// CEditor: outline: the folding of the current file as a tree at the right of the main window

void CEditor::ShowOutline( bool bShow ) {
	m_outline.m_bVisible = bShow;
	UpdateOutlineShown();
	FillOutline();
}

void CEditor::UpdateOutlineShown() {
	bool bShow = OutlineShown();
	if( bShow==m_outline.isShown() )
		return;
	if( !bShow ) {
		if( m_outline.hasFocus() ) {
			CTabPage* pPage = GetCurPage();
			::SetFocus( pPage ? pPage->sci.hWnd() : m_hWnd );
		}
		KillTimer( ID_OUTLINE );
		KillTimer( ID_OUTLINE_SYNC );
		m_outline.Show( false ); // hidden first: it is not repainted while it is cleared
		m_outline.Clear();
	}
	m_outline.Show( bShow );
	CRect r = GetClientRect();
	onSize( r.width(), r.height() );
}

int CEditor::OutlineW() {
	if( !OutlineShown() )
		return 0;
	CRect r = GetClientRect();
	int wMin = g_dpi.scale(50);
	int wMax = r.width()-g_dpi.scale(100); // the editor keeps some columns
	return max( min(m_outline.m_width, wMax), wMin );
}

CRect CEditor::OutlineSplitterRect() {
	CRect r = GetClientRect();
	int x = r.width()-OutlineW();
	return CRect( x, TOOLBAR_H, x+COutline::SplitterW(), r.height()-STATUSBAR_H ); // the full height, the results are at its left
}

void CEditor::FillOutline() {
	KillTimer( ID_OUTLINE );
	if( !OutlineShown() )
		return;
	CTabPage* pPage = GetCurPage();
	if( !pPage ) {
		m_outline.Clear();
		return;
	}
	m_outline.Fill( pPage->sci );
	m_outline.SelectLine( pPage->sci, pPage->sci.GetCurrentLine() );
}

// the folding of the editor -> the tree: it walks all the fold headers, so it is done once for many changes
// (a fold command, holding an arrow key); the item of the caret is selected at once by SCN_UPDATEUI
void CEditor::SyncOutlineLater() {
	if( OutlineShown() )
		SetTimer( ID_OUTLINE_SYNC, 50 ); // many changes at once: synced only once
}

void CEditor::SyncOutline() {
	KillTimer( ID_OUTLINE_SYNC );
	CTabPage* pPage = GetCurPage();
	if( !OutlineShown() || !pPage )
		return;
	m_outline.SyncFold( pPage->sci );
	if( !m_outline.hasFocus() ) // again: a collapsed item can be the one to select now; the focus in the outline: it moves the caret
		m_outline.SelectLine( pPage->sci, pPage->sci.GetCurrentLine() );
}

LRESULT CEditor::onNotificationOutline( NMHDR* pNMHdr ) {
	HWND hTree = m_outline.hWnd();
	switch( pNMHdr->code ) {
	case TVN_SELCHANGEDA:
	case TVN_SELCHANGEDW: // arrows or click: the line is selected, the focus stays in the outline
		if( m_outline.m_bSelectLine && !m_outline.isFilling() && ((NMTREEVIEWW*)pNMHdr)->action!=TVC_UNKNOWN )
			onOutlineGoto( false );
		break;
	case TVN_ITEMEXPANDEDA:
	case TVN_ITEMEXPANDEDW:
		if( CTabPage* pPage = GetCurPage() ) // the same folding in the editor
			m_outline.onItemExpanded( (NMTREEVIEWW*)pNMHdr, pPage->sci );
		SyncOutlineLater(); // Shift: the children are folded in the editor, the tree follows
		break;
	case NM_SETFOCUS: // the editor tabs and the results tabs are grey
		SetActiveTabs( viewOutline );
		break;
	case NM_CUSTOMDRAW: // the comments as in the editor
		return m_outline.onCustomDraw( (NMTVCUSTOMDRAW*)pNMHdr );
	case NM_CLICK: // Ctrl+click: the line is selected in the editor, the focus stays here;
		{          // on the selected item there is no TVN_SELCHANGED: select its line again
			bool bCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
			if( !bCtrl && !m_outline.m_bSelectLine )
				break;
			TVHITTESTINFO hti{};
			::GetCursorPos( &hti.pt );
			::ScreenToClient( hTree, &hti.pt );
			HTREEITEM h = TreeView_HitTest( hTree, &hti );
			if( !h || !(hti.flags & TVHT_ONITEM) )
				break;
			if( bCtrl ) {
				if( h!=TreeView_GetSelection(hTree) )
					TreeView_SelectItem( hTree, h );
				onOutlineGoto( false );
				return TRUE;
			}
			if( h==TreeView_GetSelection(hTree) )
				onOutlineGoto( false );
		}
		break;
	case NM_DBLCLK: // the line is selected in the editor, the focus goes there; the item is not expanded/collapsed
		{
			TVHITTESTINFO hti{};
			::GetCursorPos( &hti.pt );
			::ScreenToClient( hTree, &hti.pt );
			if( !TreeView_HitTest(hTree, &hti) || !(hti.flags & TVHT_ONITEM) )
				break;
		}
		onOutlineGoto( true );
		return TRUE;
	case NM_RETURN:
		onOutlineGoto( true );
		return TRUE;
	case TVN_KEYDOWN:
		if( ((NMTVKEYDOWN*)pNMHdr)->wVKey==VK_ESCAPE ) { // close the outline, the focus goes to the editor
			ShowOutline( false );
			return TRUE;
		}
		break;
	}
	return 0;
}

void CEditor::onOutlineGoto( bool bFocus ) {
	CTabPage* pPage = GetCurPage();
	if( !pPage || pPage->sci.hWnd()!=m_outline.hSci() )
		return;
	CSciWrapper& sci = pPage->sci;
	int line = m_outline.GetSelLine();
	if( line<0 || line>=sci.GetLineCount() ) // the file is changed, the outline is not rebuilt yet
		return;
	int p1 = sci.PositionFromLine( line );
	int p2 = sci.GetLineEndPosition( line );
	sci.EnsureVisible( p1, p2 ); // unfold
	int vis = sci.Call( SCI_VISIBLEFROMDOCLINE, line );
	int first = sci.GetFirstVisibleLine();
	if( vis<first || vis>=first+sci.LinesOnScreen() ) // not at the edge of the editor: a third from the top
		sci.SetFirstVisibleLine( max(vis-sci.LinesOnScreen()/3, 0) );
	sci.SetSel( p2, p1 ); // the whole line, the caret at its start: no horizontal scroll
	::SetFocus( bFocus ? sci.hWnd() : m_outline.hWnd() );
}

void CEditor::onPopupOutline( int x, int y ) {
	HWND hTree = m_outline.hWnd();
	if( x==-1 && y==-1 ) { // from the keyboard: under the selected item
		CRect r;
		auto h = TreeView_GetSelection( hTree );
		if( !h || !TreeView_GetItemRect(hTree, h, &r, TRUE) )
			r = CRect( 0, 0, 0, 0 );
		POINT pt = { r.left, r.bottom };
		::ClientToScreen( hTree, &pt );
		x = pt.x; y = pt.y;
	}
	bool bItems = TreeView_GetCount( hTree )>0;
	CMenu popup;
	popup.Create(true);
//	popup.Append( ID_VIEW_OUTLINE_SELECTLINE, tr(L"Select Line on Navigate") ); todo
//	popup.EnableCheck( ID_VIEW_OUTLINE_SELECTLINE, true, m_outline.m_bSelectLine );
	popup.Append( ID_VIEW_OUTLINE_SYNCFOLD, tr(L"Sync Folding with Editor") );
	popup.EnableCheck( ID_VIEW_OUTLINE_SYNCFOLD, true, m_outline.m_bSyncFold );
	popup.Append( ID_VIEW_OUTLINE_SHOWCLOSING, tr(L"Show Closing Lines") );
	popup.EnableCheck( ID_VIEW_OUTLINE_SHOWCLOSING, true, m_outline.m_bShowClosing );
	popup.AppendSeparator();
	popup.Append( ID_FOLDING_COLLAPSEALL, tr(L"Collapse All"), bItems );
	popup.Append( ID_FOLDING_EXPANDALL, tr(L"Expand All"), bItems );
	popup.AppendSeparator();
	popup.Append( ID_VIEW_OUTLINE, tr(L"Hide") );
	int cmd = popup.TrackPopup( TPM_RETURNCMD, x, y, m_hWnd );
	switch( cmd ) {
//	case ID_VIEW_OUTLINE_SELECTLINE: m_outline.m_bSelectLine = !m_outline.m_bSelectLine; break;
	case ID_VIEW_OUTLINE_SHOWCLOSING:
		m_outline.m_bShowClosing = !m_outline.m_bShowClosing;
		FillOutline();
		break;
	case ID_VIEW_OUTLINE_SYNCFOLD: // on: the tree gets the folding of the editor; off: the tree keeps its folding
		m_outline.m_bSyncFold = !m_outline.m_bSyncFold;
		if( m_outline.m_bSyncFold )
			FillOutline();
		break;
	case ID_FOLDING_COLLAPSEALL: // in the editor, the outline follows it; not synced: only the outline
	case ID_FOLDING_EXPANDALL:
		if( !m_outline.m_bSyncFold ) {
			m_outline.ExpandAll( cmd==ID_FOLDING_EXPANDALL );
			break;
		}
		if( CTabPage* pPage = GetCurPage() )
			pPage->sci.doFoldAll( false, cmd==ID_FOLDING_COLLAPSEALL, cmd==ID_FOLDING_EXPANDALL );
		SyncOutline();
		break;
	case ID_VIEW_OUTLINE: ShowOutline( false ); break;
	}
}
