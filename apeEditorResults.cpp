#include "stdafx.h"
#include "resource.h"
#include "apeEditor.h"
#include "options.h"
#include "lexerStyles.h"
#include "lang.h"
#include "theme.h"

// CEditor: find/replace results: the tabs of the results at the bottom of the main window

void CEditor::ShowFindResults( const string& sWhat, const string& sMatches, const vector<Sci_CharacterRange>& ranges,
		const string& sCaption, bool bNewTab ) {
	int iTab = m_tabs.GetCurSel();
	if( iTab<0 || iTab>=(int)m_vTabs.size() )
		return;
	CTabPage& page = m_vTabs[iTab];
	string sCaptionUtf = page.sci.getEncoding()==encAnsi ? a2utf(sCaption) : sCaption;
	CFindResults& res = m_results.Prepare( sCaptionUtf, bNewTab );
	res.Fill( sWhat, sMatches, page.sci, page.sFile, ranges );
	m_results.UpdateCaption( res );
	ShowResults( true );
}

void CEditor::CloseResultsTab( int i ) {
	m_findDlg.onResultsClosed( m_results.Get(i) );
	m_results.Close( i );
	if( !m_results.count() )
		ShowResults( false );
}

void CEditor::ShowResults( bool bShow ) {
	m_results.Show( bShow );
	CRect r = GetClientRect();
	onSize( r.width(), r.height() );
	if( !bShow ) {
		int iTab = m_tabs.GetCurSel();
		if( iTab>=0 && iTab<(int)m_vTabs.size() )
			::SetFocus( m_vTabs[iTab].sci.hWnd() );
	}
}

int CEditor::ResultsH() {
	if( !m_results.m_bVisible )
		return 0;
	CRect r = GetClientRect();
	int hMin = g_dpi.scale(50);
	int hMax = r.height()-TOOLBAR_H-TAB_H-STATUSBAR_H-hMin; // the editor keeps some lines
	return max( min(m_results.m_height, hMax), hMin );
}

CRect CEditor::ResultsSplitterRect() {
	CRect r = GetClientRect();
	int y = r.height()-STATUSBAR_H-ResultsH();
	return CRect( 0, y, r.width()-OutlineW(), y+CResultsTabs::SplitterH() ); // the outline is at the right
}

void CEditor::onPopupResultsTab( int i, const CPoint& pt )
{
	CMenu popup;
	popup.Create(true);
	const CFindParams& params = m_results.Get(i)->m_params;
	popup.Append( ID_SEARCH_RESULTS_RERUN, tr(L"Refresh"), !params.wsFind.empty() && !params.bReplace ); // replacing again is not a refresh
	popup.Append( ID_SEARCH_RESULTS_RERUNDLG, tr(L"Rerun..."), !params.wsFind.empty() );
	popup.AppendSeparator();
	popup.Append( ID_SEARCH_RESULTS_CLOSETAB, tr(L"Close Tab") );
	popup.Append( ID_SEARCH_RESULTS_CLOSEALL, tr(L"Close All Tabs") );
	popup.Append( ID_SEARCH_RESULTS_CLOSEOTHERS, tr(L"Close Other Tabs"), m_results.count()>1 );
	switch( popup.TrackPopup(TPM_RETURNCMD|TPM_RIGHTBUTTON, pt, m_hWnd) ) {
	case ID_SEARCH_RESULTS_RERUN:
		RerunResults( i );
		break;
	case ID_SEARCH_RESULTS_RERUNDLG: // the dialog: the search can be changed, the results go as its "new tab" says
		m_findDlg.Rerun( m_results.Get(i), true );
		break;
	case ID_SEARCH_RESULTS_CLOSETAB:
		CloseResultsTab( i );
		break;
	case ID_SEARCH_RESULTS_CLOSEOTHERS:
		for( int j = m_results.count()-1; j>=0; j-- )
			if( j!=i )
				CloseResultsTab( j );
		break;
	case ID_SEARCH_RESULTS_CLOSEALL:
		while( m_results.count() )
			CloseResultsTab( m_results.count()-1 );
		break;
	}
}

void CEditor::onNotificationResults( SCNotification* pNS ) {
	switch( pNS->nmhdr.code ) {
	case SCN_FOCUSIN:
		SetActiveTabs( viewResults );
		break;
	case SCN_UPDATEUI: // arrows or click: the result is shown, the focus stays in the results (if m_bSelectLine)
		if( m_results.m_bSelectLine && (pNS->updated & SC_UPDATE_SELECTION) && !m_results.Cur()->isFilling() && m_results.Cur()->isNewResult() )
			onResultGoto( false );
		break;
	case SCN_DOUBLECLICK: // on a file: collapse/expand; on a result: selected in the editor, the focus goes there
		if( pNS->line>=0 && m_results.Cur()->sci().IsFoldHeader( (int)pNS->line ) ) {
			m_results.Cur()->sci().ToggleFold( (int)pNS->line );
			break;
		}
		{
			CSciWrapper& sci = m_results.Cur()->sci();
			int pos = pNS->position>=0 ? (int)pNS->position : sci.GetCurrentPos();
			sci.SetSel( pos, pos ); // not the word selected by the double-click: the match at the click
			m_results.Cur()->isNewResult();
			onResultGoto( true );
		}
		break;
	}
}

void CEditor::RerunResults( int i ) {
	CFindResults* pRes = m_results.Get( i );
	if( !pRes )
		return;
	if( !pRes->m_params.bInFiles ) { // in the file of the search, if it is still opened
		int iTab = -1;
		for( size_t j=0; j<m_vTabs.size(); j++ )
			if( pRes->m_params.hSci && m_vTabs[j].sci.hWnd()==pRes->m_params.hSci )
				iTab = (int)j;
		if( iTab<0 ) {
			UpdateStatusText( tr(L"The file of the search is closed") );
			MessageBeep( MB_ICONASTERISK );
			return;
		}
		if( iTab!=m_tabs.GetCurSel() )
			SwitchToTab( iTab );
	}
	m_results.Select( i );
	m_findDlg.Rerun( pRes, false );
}

void CEditor::onResultGoto( bool bFocus ) {
	if( !m_results.Cur() )
		return;
	int iMatch;
	const CFindResult* pRes = m_results.Cur()->GetCurResult( iMatch );
	if( !pRes || iMatch<0 )
		return;
	CFindResult res = *pRes; // FileOpen() can do anything

	int iTab = -1;
	for( size_t i=0; i<m_vTabs.size(); i++ )
		if( res.hSci && m_vTabs[i].sci.hWnd()==res.hSci )
			iTab = (int)i;
	if( iTab<0 ) { // the tab is closed: open the file again, or switch to it, if it is opened again
		if( !FileOpen(res.sFile, false) )
			return;
		iTab = m_tabs.GetCurSel();
	} else
	if( iTab!=m_tabs.GetCurSel() )
		SwitchToTab( iTab );
	if( iTab<0 || iTab>=(int)m_vTabs.size() )
		return;

	// the file could be changed since the search: the line is kept, the positions are cut
	CSciWrapper& sci = m_vTabs[iTab].sci;
	const CFindMatch& m = res.matches[iMatch];
	int line = min( res.line, sci.GetLineCount()-1 );
	int len = sci.GetTextLength();
	int p1 = min( sci.PositionFromLine(line)+m.offset, len );
	int p2 = min( p1+m.len, len );
	sci.EnsureVisible( p1, p2 ); // unfold
	sci.SetSel( p1, p2 );
	::SetFocus( bFocus ? sci.hWnd() : m_results.hWndCur() );
}

void CEditor::onResultNext( bool bNext ) {
	if( m_results.isEmpty() )
		return;
	if( !m_results.m_bVisible )
		ShowResults( true );
	if( !m_results.Cur()->SelectNext(bNext) ) {
		MessageBeep( MB_OK );
		return;
	}
	m_results.Cur()->isNewResult();
	onResultGoto( true );
}
