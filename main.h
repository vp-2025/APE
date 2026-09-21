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

class CEditor : public CWindow
{
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
	bool m_bNotOneInstance;
public:
	vector<CTabPage> m_vTabs;
	CFindDlg m_findDlg;
	static bool bNewInstance;

	CEditor()
		: m_mru(ID_MRU_FIRST,ID_MRU_LAST)
		, m_pin(ID_PIN_FIRST,ID_PIN_LAST)
		, m_accelMenu(IDC_ACCEL)
		, m_findDlg(this)
	{ m_bWasZoomed=m_bSortFuncsByName=false; m_bNotOneInstance=false; }

	void onCreate( HWND hWnd );
	void onDestroy();
private:
	void ResizeScintilla();	// called when switch tab, because only current sci is resized
public:
	void onSize( int w, int h );
	void onCommand( int cmd, int notify=0, CTabPage* pPage=NULL, int iTab=0 );
private:
	void UpdateTab( int iTab );
public:
	void onNotification( NMHDR* pNMHdr );
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
	void onColors();
	void onIndicators();
	void onOptions();
	void onSort();
	void DropFiles(HDROP hdrop);

	void ViewAlwaysOnTop();
	void ViewFullScreen();
	void SetFullScreen(bool bOn);

	void onAutoComplete( CSciWrapper& sci );
	void onCallTip( CSciWrapper& sci );

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
