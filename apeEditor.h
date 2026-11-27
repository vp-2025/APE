#pragma once  

#include "sci.h"
#include "tabs.h"
#include <shellapi.h>

#include <algorithm>
#include "MRU.h"
#include "BitmapMenu.h"
#include "AccelMenu.h"
#include "dlgFind.h"
#include "tabPage.h"
#include "findResults.h"
#include "outline.h"
#include "table.h"

// the timers of the main window
#define ID_CHECK_RELOAD	1234 // timer onCheckReload
#define ID_CMB_FUNC		1235 // ctrl cmbFunc
#define ID_FILL_FUNC	1236 // timer FillFuncCombo
#define ID_TOKEN_HL		1237 // timer page.sci.DoTokenHL
#define ID_OUTLINE		1238 // timer FillOutline
#define ID_OUTLINE_SYNC	1239 // timer SyncOutline
#define ID_TABLE		1240 // timer FillTable

extern int TOOLBAR_H, STATUSBAR_H, TAB_H; // the heights of the bars, apeEditor.cpp

class CEditor : public CWindow {
	CReBar m_rebar;
	CToolbar m_toolbar;
	CStatusbar m_statusbar;
	CTabCtrlAdv m_tabs;
	CStatic m_pnl;	// shown when to tabs
	CShellImageList m_ilTabs;
	bool m_bWasZoomed;
	CMRU m_mru;
	CPinned m_pin;
	CBitmapMenu m_bmpMenu;
	CAccel2Menu m_accelMenu;
	CComboBox m_cmbFunc;
	bool m_bSortFuncsByName;
	bool m_bToolbar;	// View > Toolbar
	CResultsTabs m_results;	// at the bottom, View > Search Results
	int m_splitDrag;	// the splitter above the results is dragged: mouse y - splitter y, or -1
	COutline m_outline;	// at the right, View > Outline
	int m_splitDragV;	// the splitter left of the outline is dragged: mouse x - splitter x, or -1
	CTable m_table;	// under the editor, above the results, View > Table
	int m_splitDragT;	// the splitter above the table is dragged: mouse y - splitter y, or -1
	bool m_bTableAuto;	// the table is shown by a .csv/.tsv tab, not by the user: hidden again by another file
public:
	vector<CTabPage> m_vTabs;
	CFindDlg m_findDlg;

	CEditor()
		: m_mru(ID_MRU_FIRST,ID_MRU_LAST)
		, m_pin(ID_PIN_FIRST,ID_PIN_LAST)
		, m_accelMenu(IDC_ACCEL)
		, m_findDlg(this)
	{ m_bWasZoomed=m_bSortFuncsByName=false; m_bToolbar=true; m_splitDrag=m_splitDragV=m_splitDragT=-1; m_bTableAuto=false; }

	void onCreate( HWND hWnd, bool bNewInstance );	// bNewInstance - started via "-new"
	void onDestroy();
private:
	void ResizeScintilla();	// called when switch tab, because only current sci is resized
public:
	void onSize( int w, int h );
	void onCommand( int cmd, int notify=0, CTabPage* pPage=NULL, int iTab=0 );
private:
	void UpdateTab( int iTab );
public:
	LRESULT onNotification( NMHDR* pNMHdr );	// WM_NOTIFY: returns its result
	void onNotificationSci(SCNotification *pNS, CTabPage& page, int iTab);

	void UpdateLexersMenu( HMENU hMenu );
	void preparePopup( CMenu& menu, CTabPage* pPage=NULL );
	void onPopupLexers( CTabPage& page, const CPoint& pt );
	void onPopupFuncsSort( CTabPage& page, const CPoint& pt );
	void onPopupOverType( CTabPage& page, const CPoint& pt );
	void onPopupEncodings( CTabPage& page, const CPoint& pt );
	void onPopupEOL( CTabPage& page, const CPoint& pt );
	void onPopupShellMenu( CTabPage& page, const CPoint& pt );
	void onPopupTab( CTabPage& page, int indexPopup, const CPoint& pt );
	void onPopupStyles( CTabPage& page, const CPoint& pt );
	void onPopupSci( CTabPage& page, const CPoint& pt );
	void onPopupIncludes( CTabPage& page, const CPoint& pt );
	void onPopupStatusbar( int part, CTabPage& page );
	void onPopupGoto( CTabPage& page, const CPoint& pt );
	void onPopup( HWND hCtrl, int x, int y );

	void UpdateToolbar();
	void UpdateMenu( HMENU hMenu, CTabPage* pPage=nullptr );
	void UpdateStatusbar();
	void UpdateStatusText( const char* sz );
	void UpdateStatusText( const wchar_t* sz );
	void UpdateStatusText( const string& s ) { UpdateStatusText(s.c_str()); }
	void UpdateStatusText( const wstring& s ) { UpdateStatusText(s.c_str()); }
	void UpdateTitle();

	LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool FileOpen( const string& sFile, bool bNew, int idx=-1, int top=-1, int pos=-1 );
	void FileNew();
	void FileOpen();
private:
	void FileSave_int( int iTab, bool bSaveAs );
public:
	void FileSave( bool bSaveAs );
	void FileSaveAll( bool bDeactivate=false );

private:
	bool WindowCanClose_int(int iTab);
	bool WindowClose_int(int iTab);
	bool WindowClose_int(const string& sFile);
public:
	void WindowClose();
	void WindowCloseGroup();
	void WindowCloseExcept();
	void WindowCloseToRight();
	void WindowCloseAll();
	bool WindowCanCloseAll();

	void onTabChanging();
	void onTabChanged();
	void SwitchToTab( int iTab, bool bReset=false );

	bool onCopyData( COPYDATASTRUCT* pCDS );
	void onCheckReload();

	void onGoto( CTabPage& page, bool bGotoLine=true );
	void onColors( bool bResults=false );	// bResults: the Search Results styles are selected
	void onIndicators();
	void onOptions();
	void onSort();
	void DropFiles(HDROP hdrop);

	void onSetTheme( int iMode );
	void onThemeChanged( bool bInit=false ); // Windows dark/light mode

	void ViewAlwaysOnTop();
	void ViewFullScreen();
	void SetFullScreen(bool bOn);
	void ViewToolbar();
	void ShowToolbar(bool bShow);

	/////////////////////////////////////////////////////////////////
	// the panes: the search results, the outline, the table
	// shared by them: apeEditor.cpp
private:
	enum CFocusView { viewEditor, viewResults, viewOutline, viewTable };
	void SetActiveTabs( CFocusView view );	// the focused view has the colored current tab, the others are grey; the outline has no tabs
	bool onSplitterMouse( UINT msg, const CPoint& pt );
public:

	/////////////////////////////////////////////////////////////////
	// find/replace results: apeEditorResults.cpp
	// sCaption: the tab caption, sWhat: the start of the title; both in the encoding of the current file
	void ShowFindResults( const string& sWhat, const string& sMatches, const vector<Sci_CharacterRange>& ranges, const string& sCaption, bool bNewTab );
	void ShowResults( bool bShow );
	void CloseResultsTab( int i );
private:
	int ResultsH();	// height of the results with the splitter, 0 if hidden
	CRect ResultsSplitterRect();
	void onNotificationResults( SCNotification* pNS );
	void onResultGoto( bool bFocus );
	void onResultNext( bool bNext );
	void RerunResults( int i );	// the same search again into the results tab i, without the find dialog
	void onPopupResultsTab( int i, const CPoint& pt ); // Rerun, Close, Close Other Tabs, Close All
	void onPopupResults( int x, int y ); // in the results: Collapse All, Expand All, Hide
public:

	/////////////////////////////////////////////////////////////////
	// outline: the folding of the current file as a tree: apeEditorOutline.cpp
	void ShowOutline( bool bShow );
private:
	bool OutlineShown() { return m_outline.m_bVisible && m_tabs.GetItemCount()>0; }	// View > Outline and an editor is opened
	void UpdateOutlineShown();	// shows/hides the outline when the first tab is opened / the last one is closed
	int OutlineW();	// width of the outline with the splitter, 0 if hidden
	CRect OutlineSplitterRect();
	void FillOutline();	// for the current tab, if the outline is shown
	void SyncOutline();	// the editor folding and the caret line -> the outline
	void SyncOutlineLater();	// after a fold/caret change in the editor: ID_OUTLINE_SYNC timer
	LRESULT onNotificationOutline( NMHDR* pNMHdr );	// returns the WM_NOTIFY result
	void onOutlineGoto( bool bFocus );	// the selected item -> the line in the editor
	void onPopupOutline( int x, int y );
public:

	/////////////////////////////////////////////////////////////////
	// table: the lines of the current file split by tab/';'/',' in a list view, under the editor: apeEditorTable.cpp
	void ShowTable( bool bShow );	// by the user: View > Table, Escape, Hide
	static bool isTableFile( const string& sFile );	// .csv, .tsv, .tab: the table is shown with the file
private:
	bool TableShown() { return m_table.m_bVisible && m_tabs.GetItemCount()>0; }	// View > Table and an editor is opened
	void UpdateTableShown();	// shows/hides the table when the first tab is opened / the last one is closed
	int TableH();	// height of the table with the splitter, 0 if hidden
	CRect TableSplitterRect();
	int BottomH() { return ResultsH()+TableH(); }	// under the editor: the table and the results
	void FillTable();	// for the current tab, if the table is shown
	void AutoShowTable( const CTabPage& page );	// on a tab change: shown for a table file, hidden again for another one
	LRESULT onNotificationTable( NMHDR* pNMHdr );	// returns the WM_NOTIFY result
	void onTableGoto( int row, int col, bool bFocus );	// the cell (col<0: the line) -> selected in the editor
	void onPopupTable( int x, int y );
public:
	/////////////////////////////////////////////////////////////////

	void onAutoComplete( CSciWrapper& sci );
	void onCallTip( CSciWrapper& sci );

	CResultsTabs& Results() { return m_results; }
	CTabPage* GetCurPage() {
		int iTab = m_tabs.GetCurSel();
		return iTab>=0 && iTab<(int)m_vTabs.size() ? &m_vTabs[iTab] : nullptr;
	}
	CTabPage* FindPage( const string& sFile );	// the tab of the opened file

	CSciWrapper* GetScintilla() { 
		int iTab = m_tabs.GetCurSel();
		if( iTab<0 || iTab>=(int)m_vTabs.size() )
			return NULL;
		return &m_vTabs[iTab].sci;
	}
	bool OnGetImage(UINT menuID, HIMAGELIST& hIL, int& imageID);

private:
	void fillFuncs_int( CTabPage& page, int start, int end );
	void FillFuncs( CTabPage& page, int posFrom=0 );
	void FillFuncCombo(CTabPage& page);
	void ShowFuncCombo(CTabPage& page);
	void UpdateFuncCombo(CTabPage& page);
	void onFuncComboChange( CTabPage& page );
	void onFuncJump(CTabPage& page);

	void ReorderTabs();

	void onMove2NewApe( bool bMove );
};
