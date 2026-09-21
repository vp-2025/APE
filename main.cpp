#include "stdafx.h"
#include "resource.h"
#include "main.h"

#include "options.h"
#include "lexers.h"
#include "lexerStyles.h"

#include "dlgGoto.h"
#include "dlgIntegr.h"
#include "dlgAbout.h"
#include "dlgColors.h"
#include "dlgIndicators.h"
#include "dlgOptions.h"
#include "dlgSort.h"
#include "dlgTabs.h"
#include "dlgCheckList.h"
#include "export2HTML.h"
#include "ShellContextMenu.h"
#include "lang.h"
#include "lexNSIS.h"
#include "lexJS.h"
#include "lexCSS.h"
#include <boost/regex.hpp>
#include "sciIterator.h"

#define ID_CHECK_RELOAD	1234
#define ID_CMB_FUNC		1235
#define ID_FILL_FUNC	1236
#define ID_TOKEN_HL		1237

int TOOLBAR_H = 0; // default for 96dpi is 26
int STATUSBAR_H = 0; // default for 96dpi is 23
int TAB_H = 27;

bool CEditor::bNewInstance = false;

template<class T>
int getVectorIndex( const vector<T>& v, const T& item, int shift )
{
	for( size_t i=0; i<v.size(); ++i )
		if( &v[i]==&item )
			return i+shift;
	return -1;
}

bool OnGetImage(void* pObj, UINT menuID, HIMAGELIST& hIL, int& imageID)
{
	if( !pObj ) return false;
	return ((CEditor*)pObj)->OnGetImage(menuID, hIL, imageID);
}

bool CEditor::OnGetImage(UINT menuID, HIMAGELIST& hIL, int& imageID)
{
	string s;
	if( menuID>=ID_MRU_FIRST && menuID<=ID_MRU_LAST )
		s = m_mru.GetFile(menuID - ID_MRU_FIRST);
	else
	if( menuID>=ID_PIN_FIRST && menuID<=ID_PIN_LAST )
		s = m_pin.GetFile(menuID - ID_PIN_FIRST);
	if( s.empty() )
		return false;
	imageID = m_ilTabs.getFileTypeIndex(s);
	hIL = (HIMAGELIST)m_ilTabs;
	return true;
}

void CEditor::onCreate(HWND hWnd)
{
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) this);

	int partWidths[] = {380,40,100,120,100,-1};

	float dpiScale = GetScaleDPI();
    // no need to scale TOOLBAR_H
    // no need to scale STATUSBAR_H
    TAB_H = (int)ceil(TAB_H*dpiScale);
    for( int i=0; partWidths[i]!=-1; i++ )
        partWidths[i] = (int)(partWidths[i]*dpiScale);

	m_hWnd = hWnd;

// Toolbar ============================ todo : load from res
	TBBUTTON tbButton[] =
	{
		{0,	ID_FILE_NEW, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, -1},
		{1,	ID_FILE_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, -1},
		{2,	ID_FILE_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, -1},
		{3,	ID_FILE_SAVEAS, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, -1},
		{4,	ID_FILE_SAVEALL, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, -1},
		{-1, 0, 0, BTNS_SEP, 0, 0, 0, -1},
		{5,	ID_EDIT_UNDO, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, -1},
		{6,	ID_EDIT_REDO, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, -1},
		{-1,	0, 0, BTNS_SEP, 0, 0, 0, -1},
		{7,	ID_EDIT_CUT, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, -1},
		{8, ID_EDIT_COPY, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, -1},
		{9, ID_EDIT_PASTE, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, -1},
		{-1, 0, 0, BTNS_SEP, 0, 0, 0, -1},
		{10, ID_EDIT_READONLY, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, -1},
		{11, ID_EDIT_FORMATXML, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, -1},
		{ -1, 0, 0, BTNS_SEP, 0, 0, 0, -1 },
		{12, ID_VIEW_EOL, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, -1 },
		{13, ID_VIEW_WRAPLINES, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0, 0, -1 }
	};

	m_toolbar.Create( hWnd, IDR_TOOLBAR, sizeof(tbButton)/sizeof(TBBUTTON), tbButton, 120 );
	m_rebar.Create( hWnd );
	m_rebar.addBand( m_toolbar );
	m_cmbFunc.Create( m_toolbar.hWnd(), 0, 0, 0, 0, ID_CMB_FUNC, WS_CHILD|WS_VSCROLL|CBS_DROPDOWNLIST );
	m_cmbFunc.SetFont( m_toolbar.GetFont() );

// Statusbar ==========================
	m_statusbar.Create( hWnd, 121 );
	m_statusbar.SetParts( sizeof(partWidths)/sizeof(partWidths[0]), partWidths );

// Tabs ===============================
	m_tabs.Create( hWnd, 0,0,0,0, 222 );
	m_tabs.SetImageList( (HIMAGELIST)m_ilTabs );
	m_tabs.SetVisible( false );

	m_pnl.Create( hWnd );
	m_pnl.SetVisible( true );

// ====================================
	CWindowPlacement wp;
	GetWindowPlacement( hWnd, &wp );
	if ( !bNewInstance ) {
		CRegistry reg;
		if ( reg.Open( g_hKey, string(g_szPath)+"main" ) )
			ReadWindowPlacement( reg, wp );
	}
	SetWindowPlacement( m_hWnd, &wp );

	int lang=-1;
	LoadInt( "", "lang", lang );
	g_lang.init( ID_LANG_FIRST, ID_LANG_LAST );
	g_lang.setLang( lang );
	g_lang.translateMenu( hWnd );
	m_accelMenu.proceedMenu( hWnd );

	m_bmpMenu.AddToolbar( IDR_TOOLBAR );
	m_bmpMenu.AddToolbar( IDR_TOOLBAR_SEARCH );
	m_bmpMenu.AddToolbar( IDR_TOOLBAR_WND );
	m_bmpMenu.AddToolbar( IDR_TOOLBAR_FOLD );
	m_bmpMenu.AddToolbar( IDR_TOOLBAR_MISC );
	m_bmpMenu.SetGetImage( this, ::OnGetImage );

	UpdateToolbar();
	DragAcceptFiles( hWnd, TRUE );

	LoadLexerStyles();
	m_mru.Load();
	m_pin.Load();

	LoadBool( "main", "SortFuncsByName", m_bSortFuncsByName );
}

void CEditor::onDestroy()
{
	m_mru.Save();
	m_pin.Save();

//	SaveWindowPlacement( m_hWnd, "main" );
// vp: fix for aero snap
	CWindowPlacement wp;
	GetWindowPlacement( m_hWnd, &wp );
	if( wp.showCmd==SW_SHOWNORMAL )
		GetWindowRect( m_hWnd, &wp.rcNormalPosition );
	CRegistry reg;
	if( reg.Create( g_hKey, string(g_szPath)+"main" ) )
		WriteWindowPlacement( reg, wp );

	m_findDlg.Destroy();
	SaveBool( "main", "SortFuncsByName", m_bSortFuncsByName );
}

void CEditor::ResizeScintilla()
{
	int iTab = m_tabs.GetCurSel();
	if( iTab<0 || iTab>=(int)m_vTabs.size() )
		return;

	CRect r = GetClientRect();
	MoveWindow( m_vTabs[iTab].sci.hWnd(), 0,TAB_H+TOOLBAR_H, r.width(), r.height()-TAB_H-TOOLBAR_H-STATUSBAR_H, TRUE );
}

void CEditor::onSize( int w, int h )
{
	m_rebar.Move( 0, 0, w, 0 );
	m_statusbar.Move( 0, 0, 0, 0 );
	TOOLBAR_H = m_rebar.getHeight();
	STATUSBAR_H = m_statusbar.getHeight();
	m_tabs.Move( 0, TOOLBAR_H, w, TAB_H );
	m_pnl.Move( 0, TOOLBAR_H, w, h-TOOLBAR_H-STATUSBAR_H );

	int t = (TOOLBAR_H - m_cmbFunc.getHeight())/2;
	if( t )
		m_cmbFunc.Move( 360, t, 300, 500 );

	ResizeScintilla();
}

bool sortByName( const CTabPage& p1, const CTabPage& p2 ) {
	return compareToIC( ExtractFileName(p1.sFile), ExtractFileName(p2.sFile) )<=0;
}

bool sortByExt( const CTabPage& p1, const CTabPage& p2 ) {
	int ret = compareToIC( ExtractFileExt(p1.sFile), ExtractFileExt(p2.sFile) );
	if( ret==0 )
		ret = compareToIC( ExtractFileName(p1.sFile), ExtractFileName(p2.sFile) );
	return ret<=0;
}

bool sortByPath( const CTabPage& p1, const CTabPage& p2 ) {
	return compareToIC( p1.sFile, p2.sFile )<=0;
}

void CEditor::onMove2NewApe( bool bMove )
{
	int iTab = m_tabs.GetCurSel();
	if ( iTab < 0 ) return;
	CTabPage& tabPage = m_vTabs[iTab];
	if ( tabPage.sci.isModified() ) return;

	string sCmd;
	sCmd << GetExeFileName() <<' '<< _NEW <<" \""<< tabPage.sFile <<"\" "<< _TOP << tabPage.sci.GetFirstVisibleLine() <<' '<< _POS << tabPage.sci.GetCurrentPos();
	if( CreateProcessW( utf2w(sCmd) ) ) {
/*		CRect r;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);
		int cx = GetSystemMetrics( SM_CXSIZEFRAME )*2;
		int cy = GetSystemMetrics( SM_CYSIZEFRAME )*2;
		MoveWindow( m_hWnd, -cx, 0, r.width()/2+cx*2, r.height()+cy, TRUE );*/
		if( bMove ) {
			int iTabNew = m_tabs.GetCurSel()-1;
			WindowClose_int(iTab);
			if ( iTabNew < 0 || iTabNew >= m_tabs.GetItemCount() )
				iTabNew = 0;
			SwitchToTab( iTabNew );
		}
	}
}

void CEditor::onCommand( int cmd, int notify, CTabPage* pPage, int iTab )
{
	if( !cmd ) return;

	bool b=true;
	switch( cmd )
	{
	case ID_FILE_NEW : FileNew(); break;
	case ID_FILE_OPEN : FileOpen(); break;
	case ID_FILE_SAVE : FileSave(false); break;
	case ID_FILE_SAVEAS : FileSave(true); break;
	case ID_FILE_CLOSE : WindowClose(); break;
	case ID_FILE_SAVEALL : FileSaveAll(); break;

	case ID_FILE_EXIT: DestroyWindow(m_hWnd); break;

	case ID_WINDOW_PREV:
	case ID_WINDOW_NEXT:
		if( m_vTabs.size() > 1 ) {
            CTabsDlg dlg(m_vTabs, m_ilTabs);
			dlg.iTab = m_tabs.GetCurSel() + (cmd==ID_WINDOW_PREV ? -1 : +1);
			if( dlg.iTab>=m_tabs.GetItemCount() )
                dlg.iTab=0;
			if( dlg.iTab<0 )
                dlg.iTab=m_tabs.GetItemCount()-1;
			if( dlg.doModal( m_hWnd ) )
			    SwitchToTab(dlg.iTab);
		}
		break;

	case ID_WINDOW_CLOSE: WindowClose(); break;
	case ID_WINDOW_CLOSE_GROUP: WindowCloseGroup(); break;
	case ID_WINDOW_CLOSE_EXCEPT: WindowCloseExcept(); break;
	case ID_WINDOW_CLOSE_TORIGHT: WindowCloseToRight(); break;
	case ID_WINDOW_CLOSE_ALL: WindowCloseAll(); break;

	case ID_VIEW_ALWAYSONTOP: ViewAlwaysOnTop(); break;
	case ID_VIEW_FULLSCREEN: ViewFullScreen(); break;

	case ID_OPTIONS_INTEGR: CIntegrationDlg().doModal(m_hWnd); break;
	case ID_OPTIONS_COLORS: onColors(); break;
	case ID_OPTIONS_INDICATORS: onIndicators(); break;
	case ID_OPTIONS_OPTIONS: onOptions(); break;
	case ID_OPTIONS_NOTONEINSTANCE: m_bNotOneInstance=!m_bNotOneInstance; break;

	case ID_TAB_SORT_BYNAME:
	case ID_TAB_SORT_BYEXT:
	case ID_TAB_SORT_BYPATH: 
		if( m_vTabs.size() > 1 ) {
			int idx = m_tabs.GetCurSel();
			string sFile = m_vTabs[idx].sFile; // not ref!
			switch(cmd) {
				case ID_TAB_SORT_BYNAME: sort(m_vTabs.begin(), m_vTabs.end(), sortByName ); break;
				case ID_TAB_SORT_BYEXT:  sort(m_vTabs.begin(), m_vTabs.end(), sortByExt  ); break;
				case ID_TAB_SORT_BYPATH: sort(m_vTabs.begin(), m_vTabs.end(), sortByPath ); break;
			}
			for( size_t i=0; i < m_vTabs.size(); i++ )
				UpdateTab(i);
			for( size_t i=0; i < m_vTabs.size(); i++ )
				if( m_vTabs[i].sFile == sFile ) {
					m_tabs.SetCurSel(i);
					break;
				}
		}
		break;

	case ID_HELP_ABOUT: CAboutDlg().doModal(m_hWnd); break;
	default:
		b=false;
	}
	if( b ) return;

// MRU { ============
	if( cmd==ID_MRU_CLEAR ) { m_mru.Clear(); return; }
	if( cmd==ID_MRU_OPENALL )
	{
		m_mru.Pause();
		for( int i=0; i<m_mru.GetSize(); i++ )
			FileOpen( m_mru.GetFileWithRemove(i,false), false );
		m_mru.Continue();
		return;
	}
	if( cmd>=ID_MRU_FIRST && cmd<=ID_MRU_LAST ) 
	{ 
		FileOpen( m_mru.GetFileWithRemove(cmd-ID_MRU_FIRST,false), false ); 
		return; 
	}
// ================== }

// Pinned { ============
	if( cmd==ID_PIN_CLEAR ) { m_pin.Clear(); return; }
	if( cmd==ID_PIN_OPENALL )
	{
		for( int i=0; i<m_pin.GetSize(); i++ )
			FileOpen( m_pin.GetFileWithRemove(i,true), false );
		return;
	}
	if( cmd>=ID_PIN_FIRST && cmd<=ID_PIN_LAST )
	{
		FileOpen( m_pin.GetFileWithRemove(cmd-ID_PIN_FIRST,true), false );
		return;
	}
// ================== }

// Languages { ============
	if( cmd>=ID_LANG_FIRST && cmd<=ID_LANG_LAST )
	{
		int lang = g_lang.langFromCmd(cmd);
		SaveInt( "", "lang", lang ) ;
		g_lang.setLang( lang );

		HMENU hMenu = LoadMenu( g_hInst, MAKEINTRESOURCE(IDC_MENU) );
		HMENU hMenuOld = GetMenu( m_hWnd );
		SetMenu( m_hWnd, hMenu );
		DestroyMenu( hMenuOld );
		g_lang.translateMenu( m_hWnd );
		m_accelMenu.proceedMenu( m_hWnd );

		m_findDlg.Destroy();
		return;
	}
// ================== }

	if( !pPage ) {
		iTab = m_tabs.GetCurSel();
		if( iTab>=0 && iTab<(int)m_vTabs.size() )
			pPage = &m_vTabs[iTab];
	}
	if( !pPage )
		return;
	CTabPage& page = *pPage;

	if( cmd>=ID_LEXER_FIRST && cmd<=ID_LEXER_LAST )
	{
		int lexer = cmd-ID_LEXER_FIRST;
		if( lexer==page.sci.Lexer() ) return;
		page.sci.SetLexer( lexer );
		SetupLexerStyles(page.sci);
		page.sci.ReStyle();
		ShowFuncCombo(page);
		UpdateStatusbar();
		return;
	}

	switch( cmd )
	{
	case ID_SEARCH_FIND:
	case ID_SEARCH_FINDFILE:
	case ID_SEARCH_REPLACE:
	case ID_SEARCH_REPLACEFILE:
		m_findDlg.Find( cmd==ID_SEARCH_REPLACE||cmd==ID_SEARCH_REPLACEFILE, cmd==ID_SEARCH_FINDFILE||cmd==ID_SEARCH_REPLACEFILE );
		break;

	case ID_SEARCH_FINDPREV:
	case ID_SEARCH_FINDNEXT:
		m_findDlg.FindNext( cmd==ID_SEARCH_FINDNEXT );
		break;
	case ID_SEARCH_CLEARINDIC:
		m_findDlg.ClearIndicators();
		break;

	case ID_SEARCH_GOTO: onGoto(page); break;
	case ID_SEARCH_MATCHBRACE: page.sci.MatchBrace(); break;
	case ID_SEARCH_MATCHFOLD: page.sci.MatchFold(); break;

	case ID_EDIT_UNDO: page.sci.Undo(); break;
	case ID_EDIT_REDO: page.sci.Redo(); break;
	case ID_EDIT_CUT: page.sci.Cut(); break;
	case ID_EDIT_COPY: page.sci.Copy(); break;
	case ID_EDIT_COPYASHTML: page.sci.CopyAsHTML(); break;
	case ID_EDIT_COPYASHEX: page.sci.CopyAsHex(); break;
	case ID_EDIT_PASTE: page.sci.Paste(); break;
	case ID_EDIT_DELETE: page.sci.Delete(); break;
	case ID_EDIT_SELECTALL: page.sci.SelectAll(); break;

	case ID_EDIT_READONLY: 
		{
			if( page.sci.IsReadOnly() ) {
				DWORD dw = GetFileAttrs(page.sFile);
				dw &= ~FILE_ATTRIBUTE_READONLY;
				if( !SetFileAttrs( page.sFile, dw ) )
					break;
			}
			page.sci.SwitchReadOnly(); 
			UpdateToolbar();
			m_tabs.Invalidate();
		}
		break;

	case ID_EDIT_UPPERCASE: page.sci.UpperCase(); break;
	case ID_EDIT_LOWERCASE: page.sci.LowerCase(); break;

	case ID_EDIT_AUTOC: onAutoComplete(page.sci); break;
	case ID_EDIT_CALLTIP: onCallTip(page.sci); break;

	case ID_EDIT_FORMATINDENT: page.sci.FormatIndent(); break;
	case ID_EDIT_FORMATXML : page.sci.FormatXml(); break;
	case ID_EDIT_WRAPLINES: page.sci.WrapLines(); break;
	case ID_EDIT_JUMP : onFuncJump(page); break;
	case ID_EDIT_SORT : onSort(); break;
	case ID_EDIT_STRIPTRAILINGSPACES: page.sci.stripTrailingSpaces(true); break;

	case ID_EOL_CRLF : page.sci.setEOLMode(SC_EOL_CRLF); break;
	case ID_EOL_CR : page.sci.setEOLMode(SC_EOL_CR); break;
	case ID_EOL_LF : page.sci.setEOLMode(SC_EOL_LF); break;

	case ID_ENCODING_ANSI:
		page.sci.setEncodingAndConvert(encAnsi); UpdateTab(iTab); UpdateStatusbar(); break;
	case ID_ENCODING_UTF8:
		page.sci.setEncodingAndConvert(encUtf8); UpdateTab(iTab); UpdateStatusbar(); break;
	case ID_ENCODING_UTF8_NO_BOM:
		page.sci.setEncodingAndConvert(encUtf8NoBOM); UpdateTab(iTab); UpdateStatusbar(); break;
	case ID_ENCODING_UTF16BE:
		page.sci.setEncodingAndConvert(encUtf16BE); UpdateTab(iTab); UpdateStatusbar(); break;
	case ID_ENCODING_UTF16LE:
		page.sci.setEncodingAndConvert(encUtf16LE); UpdateTab(iTab); UpdateStatusbar(); break;
	case ID_ENCODING_LATIN1:
		page.sci.setEncodingAndConvert(encLatin1); UpdateTab(iTab); UpdateStatusbar(); break;
	case ID_ENCODING_LATIN2:
		page.sci.setEncodingAndConvert(encLatin2); UpdateTab(iTab); UpdateStatusbar(); break;
	case ID_ENCODING_LATIN3:
		page.sci.setEncodingAndConvert(encLatin3); UpdateTab(iTab); UpdateStatusbar(); break;
	case ID_ENCODING_ANSI_CYR:
		page.sci.setEncodingAndConvert(encAnsiCyr); UpdateTab(iTab); UpdateStatusbar(); break;

    case ID_EDIT_VP: {
/*        replaceSciRegExp(&page.sci, "^[_a-zA-Z0-9]+.h$", "");
        replaceSciRegExp(&page.sci, "^[_a-zA-Z0-9]+.cpp$", "");
        replaceSciRegExp(&page.sci, R"(^[-\.\:(){}\[\]<>!|&:,"_+*=\t a-zA-Z0-9]*getProfileString)", "getProfileString");
        replaceSciRegExp(&page.sci, R"(^[-\.\:(){}\[\]<>!|&:,"_+*=\t a-zA-Z0-9]*getProfileInt)", "getProfileInt");
        replaceSciRegExp(&page.sci, R"(^[-\.\:(){}\[\]<>!|&:,"_+*=\t a-zA-Z0-9]*getProfileBool)", "getProfileBool");
        replaceNormal(&page.sci, "getProfileInt( ", "getProfileInt(");
        replaceNormal(&page.sci, "getProfileBool( ", "getProfileBool(");
        replaceNormal(&page.sci, "getProfileString( ", "getProfileString(");
        sortLines(page.sci, true, true, true); */

/*        replaceSciRegExp(&page.sci, "^#define ", "const ");
        replaceSciRegExp(&page.sci, "([0-9]+)$", "= \\1"); */
    }; break;

	case ID_TAB_SETCURDIR :
		SetCurDir( ExtractFilePath(page.sFile) );
		break;
	case ID_TAB_OPENFOLDER:
		OpenFolder( ExtractFilePath(page.sFile), page.sFile );
		break;

	case ID_TAB_PIN : m_pin.doPin(page.sFile); break;

	case ID_TAB_MOVE2NEWAPE:
	case ID_TAB_OPEN2NEWAPE:
		onMove2NewApe( cmd==ID_TAB_MOVE2NEWAPE );
		break;

	case ID_VIEW_WS_NONE: 
	case ID_VIEW_WS_PART:
	case ID_VIEW_WS_ALL: 
		switch( cmd ) {
			case ID_VIEW_WS_NONE: g_options.iViewWS = SCWS_INVISIBLE; break;
			case ID_VIEW_WS_PART: g_options.iViewWS = SCWS_VISIBLEAFTERINDENT; break;
			case ID_VIEW_WS_ALL:  g_options.iViewWS = SCWS_VISIBLEALWAYS; break;
		}
		g_options.Save();
		for(auto& tabPage : m_vTabs)
            tabPage.sci.SetViewWS(g_options.iViewWS);
		break;
	case ID_VIEW_EOL: 
		g_options.bViewEOL = !page.sci.GetViewEOL();
		g_options.Save();
		for(auto& tabPage : m_vTabs)
            tabPage.sci.SetViewEOL(g_options.bViewEOL);
		UpdateToolbar();
		break;
	case ID_VIEW_BRACE: 
		g_options.bViewBraceHL = !page.sci.isBraceHL();
		g_options.Save();
		for(auto& tabPage : m_vTabs)
            tabPage.sci.BraceHL(g_options.bViewBraceHL);
		break;
	case ID_VIEW_NUMS: 
		g_options.bViewNums = !page.sci.isViewNums();
		g_options.Save();
		for(auto& tabPage : m_vTabs)
            tabPage.sci.ViewNums(g_options.bViewNums);
		break;
	case ID_VIEW_INDENT: 
		g_options.bViewIndentGuides = !page.sci.isViewIndentGuides();
		g_options.Save();
		for(auto& tabPage : m_vTabs)
            tabPage.sci.ViewIndentGuides(g_options.bViewIndentGuides);
		break;
	case ID_VIEW_LINEHL: 
		g_options.bViewLineHL = !page.sci.GetCarretLineVisible();
		g_options.Save();
		for(auto& tabPage : m_vTabs)
            tabPage.sci.SetCarretLineVisible(g_options.bViewLineHL);
		break;
	case ID_VIEW_XMLTAGS: 
		g_options.bViewXMLMatchTagHL = !page.sci.isXmlMatchedTagHL();
		g_options.Save();
		for(auto& tabPage : m_vTabs)
            tabPage.sci.setXmlMatchedTagHL(g_options.bViewXMLMatchTagHL);
		break;
	case ID_VIEW_TOKEN: 
		g_options.bViewTokenHL = !page.sci.isTokenHL();
		g_options.Save();
		for(auto& tabPage : m_vTabs)
            tabPage.sci.setTokenHL(g_options.bViewTokenHL);
		break;

	case ID_VIEW_WRAPLINES: 
		g_options.bViewWrapLongLines = !g_options.bViewWrapLongLines;
		g_options.Save();
		for(auto& tabPage : m_vTabs)
            tabPage.sci.setWrapLongLines(g_options.bViewWrapLongLines);
		break;

	case ID_VIEW_ZOOMIN: page.sci.ZoomIn(); break;
	case ID_VIEW_ZOOMOUT: page.sci.ZoomOut(); break;
	case ID_VIEW_ZOOMRESET: page.sci.SetZoom(0); break;

	case ID_FOLDING_TOGGLE:
	case ID_FOLDING_COLLAPSE:
	case ID_FOLDING_EXPAND:
		page.sci.doFold( cmd==ID_FOLDING_COLLAPSE, cmd==ID_FOLDING_EXPAND );
		break;

	case ID_FOLDING_TOGGLECHILD:
	case ID_FOLDING_COLLAPSECHILD:
	case ID_FOLDING_EXPANDCHILD:
		page.sci.doFoldChild( -1, cmd==ID_FOLDING_TOGGLECHILD, cmd==ID_FOLDING_COLLAPSECHILD, cmd==ID_FOLDING_EXPANDCHILD );
		break;

	case ID_FOLDING_TOGGLEALL:
	case ID_FOLDING_COLLAPSEALL:
	case ID_FOLDING_EXPANDALL:
		page.sci.doFoldAll( cmd==ID_FOLDING_TOGGLEALL, cmd==ID_FOLDING_COLLAPSEALL, cmd==ID_FOLDING_EXPANDALL );
		break;

	case ID_FILE_EXPORT2HTML:
		{
			string sFile = page.sFile + ".html";
			if( dlgSaveFile( m_hWnd, sFile ) )
				export2HTML( page.sci, sFile );
		}
		break;
	case ID_CMB_FUNC:
		if (notify == CBN_SELCHANGE) {
			if ( m_cmbFunc.SendMessage(CB_GETDROPPEDSTATE)==FALSE )
				onFuncComboChange(page);
		} else
		if( notify == CBN_CLOSEUP )
			onFuncComboChange(page);
		break;
	}
}

void CEditor::UpdateTab( int iTab ) {
	CTabPage& page = m_vTabs[iTab];
	string sFile = ExtractFileName(page.sFile);
	if( page.sci.isModified() )
		sFile += '*';
	m_tabs.UpdateItem(iTab, utf2w(sFile), m_ilTabs.getFileTypeIndex(page.sFile) );
}

void MarginClick( CSciWrapper& sci, int pos )
{
	int line = sci.LineFromPosition(pos);
	if( sci.IsFoldHeader(line) )
	{
		if( (GetAsyncKeyState(VK_SHIFT)&0x8000)==0 )
			sci.ToggleFold(line);
		else
			sci.doFoldChild(line, true, false, false);
	}
}

void HyperlinkClick( CSciWrapper& sci, int pos )
{
	int style = sci.GetStyleAt(pos); // link has style 3 or 6; so we can use styling
	int p1 = pos, p2 = pos;
	for( ; p1>=0 && sci.GetStyleAt(p1)==style; --p1 );
	for( ; p2<sci.GetLength() && sci.GetStyleAt(p2)==style; ++p2 );
	string s = sci.GetTextRange(p1+1,p2);
	if ( s.empty() ) return;
	if( (GetAsyncKeyState(VK_CONTROL)&0x8000)!=0 )
		CreateProcess( "C:/Program Files (x86)/Google/Chrome/Application/chrome.exe --incognito \"" + s + "\"" );
	else
		ShellExecute( sci.hWnd(), "open", s.c_str(), NULL, NULL, SW_SHOWNORMAL );
}

void CEditor::onNotificationSci(SCNotification *pNS, CTabPage& page, int iTab)
{
	switch(pNS->nmhdr.code)
	{
		case SCN_CHARADDED:
			UpdateStatusbar();
			page.sci.CheckIndent(pNS);
			if( pNS->ch=='.' )
				onAutoComplete(page.sci);
			break;
		case SCN_MODIFIED:
			page.sci.DoTokenHLSkip();
			page.sci.AdjustNumsMargin();
			if( page.sci.GetUndoCollection() ) // not in loadFromFile
			{
				KillTimer( ID_FILL_FUNC );
				SetTimer( ID_FILL_FUNC, 3000 );

				int line = page.sci.LineFromPosition( pNS->position );
				if( line<page.sci.GetLineCount()-1 && !page.sci.Call( SCI_GETLINEVISIBLE, line+1) )
					page.sci.doFoldChild( line, false, false, true );
				else
				if ( line>1 && !page.sci.Call( SCI_GETLINEVISIBLE, line-1 ) ) {
					int lineP = page.sci.GetFoldParent( line );
					if( lineP>=0 && lineP<line )
						page.sci.doFoldChild( lineP, false, false, true );
				}
			}
			break;
		case SCN_UPDATEUI:
			page.sci.DoBraceHL();
			UpdateStatusbar();
			UpdateToolbar();
			UpdateFuncCombo(page);
			page.sci.XmlMatchedTagHL();
			{
				KillTimer( ID_TOKEN_HL );
				SetTimer( ID_TOKEN_HL, 500 );
			}
			break;
		case SCN_MARGINCLICK:
			if( pNS->margin==MARGIN_FOLD )
				MarginClick( page.sci, pNS->position ); // pNS->modifiers
			break;
		case SCN_HOTSPOTCLICK:
			HyperlinkClick( page.sci, pNS->position );
			break;		
		case SCN_SAVEPOINTREACHED:
		case SCN_SAVEPOINTLEFT:
			UpdateTab( iTab );
			break;
		case SCN_STYLENEEDED:
			onStyleNeeded( page.sci, page.sFile, pNS->position );
			break;
	}
}

void CEditor::onNotification( NMHDR* pNMHdr )
{
	if( pNMHdr->hwndFrom==m_tabs.hWnd() ) {
		switch( pNMHdr->code ) {
			case TCN_SELCHANGING: onTabChanging(); break;
			case TCN_SELCHANGE: onTabChanged(); break;
		}
	} else {
		for( unsigned i=0; i < m_vTabs.size(); i++ )
			if( pNMHdr->hwndFrom == m_vTabs[i].sci.hWnd() )
				onNotificationSci((SCNotification*)pNMHdr, m_vTabs[i], i );
	}
}

void AppendWord( CMenu& popup, int cmd, const string& sWord, const wchar_t* szFrom, const wchar_t* szTo )
{
	wstring s = popup.TextW( cmd );
	size_t pos = s.find(szFrom);
	size_t pos2 = s.rfind(szTo);
	if( pos!=string::npos && pos2!=string::npos )
		s.erase( s.begin()+pos, s.begin()+pos2 );
	if( !sWord.empty() )
	{
		wstring s1;
		s1 << szFrom << utf2w(sWord) << szTo;
		pos = s.find( '\t' );
		if( pos==string::npos )
			s += s1;
		else
			s.insert( pos, s1 );
	}
	popup.Text( cmd, s );
}

void FillLexersMenu( CMenu& popup, CTabPage* pPage )
{
	for( CLexerInfo* pLex=g_lexers; pLex->iLexerID>0; pLex++ )
	{
		if( pLex->iLexerID==LEX_SEPARATOR )
			popup.AppendSeparator();
		else {
			int cmd = ID_LEXER_FIRST + pLex->iLexerID;
			popup.Append( cmd, pLex->szLexerName );
			popup.EnableCheck( cmd, pPage!=NULL, pPage && pPage->sci.Lexer()==pLex->iLexerID );
		}
	}
}

void CEditor::UpdateLexersMenu( HMENU hMenu )
{
	CTabPage* pPage = NULL;
	int iTab = m_tabs.GetCurSel();
	if( iTab>=0 && iTab<(int)m_vTabs.size() )
		pPage = &m_vTabs[iTab];

	CMenu popup;
	popup.Attach( hMenu );
	while( popup.GetItemCount() )
		popup.DeleteByPos(0);

	FillLexersMenu( popup, pPage );
}

void CEditor::onPopupLexers( CTabPage& page, const CPoint& pt )
{
	CMenu popup;
	popup.Create(true);
	FillLexersMenu( popup, &page );
	int cmd = popup.TrackPopup( TPM_RETURNCMD|TPM_RIGHTBUTTON, pt, m_hWnd );
	onCommand(cmd);
}

void CEditor::preparePopup( CMenu& menu, CTabPage* pPage )
{
	g_lang.translateMenu( menu.hMenu() );
	m_accelMenu.proceedMenu( menu.hMenu() );
	UpdateMenu( menu.hMenu(), pPage );
}

void CEditor::onPopupOverType( CTabPage& page, const CPoint& pt )
{
	CSciWrapper& sci = page.sci;
	CMenu menu(IDC_POPUP_OVR);
	menu.EnableCheck( ID_OVR, true, sci.GetOverType() );
	menu.EnableCheck( ID_INS, true, !sci.GetOverType() );
	preparePopup( menu );
	int cmd = menu.TrackPopup( TPM_RETURNCMD|TPM_RIGHTBUTTON, pt, m_hWnd );
	switch( cmd ) {
		case ID_OVR: sci.SetOverType(true); break;
		case ID_INS: sci.SetOverType(false); break;
	}
	UpdateStatusbar();
}

void CEditor::onPopupFuncsSort( CTabPage& page, const CPoint& pt )
{
	CMenu menu(IDC_POPUP_FUNC_COMBO);
	menu.EnableCheck( ID_SORT_BY_NAME, true, m_bSortFuncsByName );
	menu.EnableCheck( ID_SORT_BY_LINE, true, !m_bSortFuncsByName );
	preparePopup( menu );
	int cmd = menu.TrackPopup( TPM_RETURNCMD|TPM_RIGHTBUTTON, pt, m_hWnd );
	switch( cmd ) {
	case ID_SORT_BY_NAME:
		if( !m_bSortFuncsByName ) {
			m_bSortFuncsByName = true;
			for(auto& tabPage : m_vTabs)
                tabPage.vFunc.sortByName();
			FillFuncCombo(page);
		} break;
	case ID_SORT_BY_LINE:
		if( m_bSortFuncsByName ) {
			m_bSortFuncsByName = false;
			for(auto& tabPage : m_vTabs)
				tabPage.vFunc.sortByLine();
			FillFuncCombo(page);

		} break;
	}
}

void CEditor::onPopupEncodings( CTabPage& page, const CPoint& pt )
{
	CMenu menu(IDC_POPUP_ENCODING);
	preparePopup( menu );
	int cmd = menu.TrackPopup( TPM_RETURNCMD|TPM_RIGHTBUTTON, pt, m_hWnd );
	onCommand(cmd);
}

void CEditor::onPopupEOL( CTabPage& page, const CPoint& pt )
{
	CMenu menu(IDC_POPUP_EOL);
	preparePopup( menu );
	int cmd = menu.TrackPopup( TPM_RETURNCMD|TPM_RIGHTBUTTON, pt, m_hWnd );
	onCommand(cmd);
}

bool recalcPath(string& sPath)
{
	vector<string> v;
	replaceStr( sPath, "/", "\\" );
	split(sPath, v, '\\');

	for( size_t i=0; i<v.size(); )
		if( v[i]=="." )
			v.erase( v.begin()+i );
		else
			++i;
	for( size_t i=0; i<v.size(); )
	{
		if( v[i]==".." )
		{
			if( i==0 )
				return false;
			v.erase( v.begin()+i-1, v.begin()+i+1 );
		} else
			++i;
	}


#ifdef DEBUG
	for( const string& s : v )
		if( s=="." || s==".." )
			return false;
#endif

	sPath = combine( v, '\\' );
	return true;
}


void CEditor::onPopupShellMenu( CTabPage& page, const CPoint& pt )
{
	string sFile = page.sFile;
	if( !recalcPath(sFile) )
		return;
	CShellContextMenu m;
	m.SetObject( sFile );
	m.ShowContextMenu( m_hWnd, pt );
}

void CEditor::onPopupTab( CTabPage& page, int iTab, const CPoint& pt )
{
	CMenu menu( IDC_POPUP_TAB );
	preparePopup( menu, &page );
	string sAlternate = doAlternate(m_vTabs[iTab].sFile);
	bool canAlternate = !sAlternate.empty();
	bool canSelect = iTab!=m_tabs.GetCurSel();
	if( !canSelect )
		menu.Delete( ID_TAB_SELECT );
	if( !canAlternate ) {
		menu.Delete( ID_TAB_ALTERNATE );
		menu.Delete( ID_WINDOW_CLOSE_GROUP ); // is opened?
	}

	int cmd = menu.TrackPopup( TPM_RETURNCMD|TPM_RIGHTBUTTON, pt, m_hWnd );
	switch(cmd) {
		case ID_TAB_SELECT :
			if( canSelect )
				SwitchToTab( iTab );
			break;
		case ID_TAB_ALTERNATE :
			if( canAlternate )
				FileOpen( sAlternate, false );
			break;
		case ID_TAB_SHELLMENU:
			onPopupShellMenu( page, pt );
			break;

		case ID_WINDOW_CLOSE:
		case ID_WINDOW_CLOSE_EXCEPT:
		case ID_WINDOW_CLOSE_GROUP:
		case ID_WINDOW_CLOSE_TORIGHT:
			if( canSelect )
				SwitchToTab( iTab );

		case ID_TAB_MOVE2NEWAPE:
			// SwitchToTab(iTab);
			onCommand( cmd, 0, &page, iTab );
			break;

		default:
			onCommand( cmd, 0, &page, iTab );
	}
}

void CEditor::onPopupStyles( CTabPage& page, const CPoint& pt )
{
	CMenu menu( IDC_POPUP_STYLE );

	string sWord;
	bool hasSelection = page.sci.hasSelection(100);
	if( hasSelection )
		sWord = page.sci.GetSelText();
	else
		sWord = page.sci.GetWordFromPoint(pt.x,pt.y);
	bool bAny=false;
	for( int i=0; i<INDIC_CNT; i++ )
	{
		AppendWord( menu, ID_EDIT_STYLE1+i, page.sci.getHighlighted(i), L" '", L"'" );
		AppendWord( menu, ID_EDIT_CLEAR_STYLE1+i, page.sci.getHighlighted(i), L" '", L"'" );
		menu.Enable( ID_EDIT_STYLE1+i, !sWord.empty() );
		menu.Enable( ID_EDIT_CLEAR_STYLE1+i, page.sci.isHighlighted(i) );
		bAny |= page.sci.isHighlighted(i);
	}
	menu.Enable( ID_EDIT_RESTYLEALL, bAny );
	menu.Enable( ID_EDIT_CLEAR_STYLE_ALL, bAny );

	g_lang.translateMenu( menu.hMenu() );
	m_accelMenu.proceedMenu( menu.hMenu() );
	UpdateMenu( menu.hMenu() );
	int cmd = menu.TrackPopup( TPM_RETURNCMD|TPM_RIGHTBUTTON, pt, m_hWnd );

	if( ID_EDIT_STYLE1<=cmd && cmd<=ID_EDIT_STYLE7 ) {
		int cnt = page.sci.Highlight( sWord, !hasSelection, cmd-ID_EDIT_STYLE1 );
		UpdateStatusText( string() <<"Styled: "<< cnt );
	} else
	if( ID_EDIT_CLEAR_STYLE1<=cmd && cmd<=ID_EDIT_CLEAR_STYLE7 )
		page.sci.ClearHighlight( cmd-ID_EDIT_CLEAR_STYLE1 );
	else
	if( cmd==ID_EDIT_CLEAR_STYLE_ALL )
		page.sci.ClearHighlight(-1);
	else
	if( cmd==ID_EDIT_RESTYLEALL )
		page.sci.ReHighlight();
	else
		onCommand(cmd);
}

void CEditor::onPopupSci( CTabPage& page, const CPoint& pt )
{
	CMenu menu( IDC_POPUP_SCI );

	bool hasSelection = page.sci.hasSelection(100);
	string sPath = hasSelection ? page.sci.GetSelText() : page.sci.GetFileNameFromPoint(pt.x,pt.y);
	EnsureFilePath( sPath, ExtractFilePath(page.sFile) );
	if( IsFileExists(sPath) )
		AppendWord( menu, ID_EDIT_OPENFILE, sPath, L" (", L")" );
	else
		menu.Delete( ID_EDIT_OPENFILE );
	if( IsDirExists(sPath) )
		AppendWord( menu, ID_EDIT_OPENDIR, sPath, L" (", L")" );
	else
		menu.Delete( ID_EDIT_OPENDIR );

	string sFunc = page.sci.GetWordFromPoint(pt.x,pt.y);
	AppendWord( menu, ID_EDIT_JUMP, sFunc, L" (", L")" );
	menu.Enable( ID_EDIT_JUMP, !sFunc.empty() );

	g_lang.translateMenu( menu.hMenu() );
	m_accelMenu.proceedMenu( menu.hMenu() );
	UpdateMenu( menu.hMenu() );
	int cmd = menu.TrackPopup( TPM_RETURNCMD|TPM_RIGHTBUTTON, pt, m_hWnd );
	switch( cmd )
	{
		case ID_EDIT_OPENFILE:
			FileOpen( sPath, false, getVectorIndex(m_vTabs, page, 1) );
			break;
		case ID_EDIT_OPENDIR:
			OpenFolder( sPath );
			break;
		case ID_EDIT_JUMP: onFuncJump(page); break;
		case ID_EDIT_STYLES: onPopupStyles( page, pt ); break;
		case ID_EDIT_INCLUDES: onPopupIncludes( page, pt ); break;
	default:
		onCommand(cmd);
	}
}

void CEditor::onPopupGoto(CTabPage& page, const CPoint& pt)
{
	CMenu menu( IDC_POPUP_GOTO );
	g_lang.translateMenu( menu.hMenu() );
	m_accelMenu.proceedMenu( menu.hMenu() );
	UpdateMenu( menu.hMenu() );
	int cmd = menu.TrackPopup( TPM_RETURNCMD|TPM_RIGHTBUTTON, pt, m_hWnd );
	switch( cmd ) {
		case ID_GOTO_LINE: onGoto( page, true ); break;
		case ID_GOTO_POSITION: onGoto( page, false ); break;
	}
}

struct TextToFindV : public Sci_TextToFind {
public:
	TextToFindV( const char* szText, int min, int max )
	{
		lpstrText = (char*)szText;
		chrg.cpMin = min;
		chrg.cpMax = max;
		chrgText.cpMin = chrgText.cpMax = 0;
	}
};

string findStrRE( const string& s, const char* szRE)
{
	boost::regex expr( szRE );
	boost::match_results<string::const_iterator> what;
	if( boost::regex_search(s, what, expr ) )
		return what[1];
	return {};
}

struct CInclude {
	string first, second;
	int pos;
	CInclude( const string& first_, const string& second_, int pos_ )
	{ first=first_; second=second_; pos=pos_; }
};

void parseIncludes(CSciWrapper& sci, vector<CInclude>& v)
{
	int flags = SCFIND_REGEXP|SCFIND_POSIX;
	if( sci.Lexer()==LEX_XHTML )
	{
		{
			TextToFindV ttfOpen("<link[ \t]", 0, sci.GetTextLength());
			while( sci.FindText( flags, ttfOpen )!=-1 )
			{
				TextToFindV ttfClose(">", ttfOpen.chrgText.cpMin, sci.GetTextLength() );
				if( sci.FindText( flags, ttfClose )==-1 ) 
					break;
				string s = sci.GetTextRange( ttfOpen.chrgText.cpMax, ttfClose.chrgText.cpMin );
				string type = findStrRE(s,"type[ \t]*=[ \t]*\"([^\"]*)\"");
				string href = findStrRE(s,"href[ \t]*=[ \t]*\"([^\"]*)\"");
				if( !href.empty() )
					v.push_back( CInclude(type, href, ttfOpen.chrgText.cpMax) );
				ttfOpen.chrg.cpMin = ttfClose.chrgText.cpMax;
			}
		}
		{
			TextToFindV ttfOpen("<script[ \t]", 0, sci.GetTextLength());
			while( sci.FindText( flags, ttfOpen )!=-1 )
			{
				TextToFindV ttfClose(">", ttfOpen.chrgText.cpMin, sci.GetTextLength() );
				if( sci.FindText( flags, ttfClose )==-1 ) 
					break;
				string s = sci.GetTextRange( ttfOpen.chrgText.cpMax, ttfClose.chrgText.cpMin );
				string type = findStrRE(s,"type[ \t]*=[ \t]*\"([^\"]*)\"");
				string src = findStrRE(s,"src[ \t]*=[ \t]*\"([^\"]*)\"");
				if( !src.empty() )
					v.push_back( CInclude(type, src, ttfOpen.chrgText.cpMax) );
				ttfOpen.chrg.cpMin = ttfClose.chrgText.cpMax;
			}
		}
	}
	if( sci.Lexer()==LEX_XHTML || sci.Lexer()==LEX_JS )
	{
		int start = 0;
		CSciIterator itB(&sci,sci.GetTextLength());
		boost::regex expr( R"(([a-zA-Z0-9][a-zA-Z0-9/\\-]*\.htm[l]?)[^A-Za-z0-9]+)" );
		while( true )
		{
			boost::match_results<CSciIterator> what;
			CSciIterator itA(&sci,start);
			if( !boost::regex_search(itA, itB, what, expr ) )
				break;
			start = what[0].second.getPos();
			string html = GetTextRange(what[1]);
			if( !html.empty() )
				for( size_t i=0; i<v.size(); i++ )
					if( v[i].first.empty() && v[i].second==html ) {
						html.clear();
						break;
					}
			if( !html.empty() )
				v.emplace_back(string(), html, what[1].first.getPos() );
		}
	}
}

void CEditor::onPopupIncludes(CTabPage& page, const CPoint& pt)
{
	vector<CInclude> vIncludes;
	parseIncludes(page.sci, vIncludes);

	CMenu popup;
	popup.Create(true);
	if( vIncludes.empty() ) {
		popup.Append(0,"(None)");
		popup.Enable(0,false);
	}
	for( size_t i=0; i<vIncludes.size(); i++ ) {
		string sUtf;
		if ( page.sci.isEncodingUtf() )
			sUtf = vIncludes[i].second;
		else
			sUtf = a2utf(vIncludes[i].second);
		bool bFileExists = IsFileExists( ExtractFilePath( page.sFile ) + sUtf );
		if( !vIncludes[i].first.empty() )
			sUtf <<" ("<< vIncludes[i].first <<")";
		popup.Append( 100+i, utf2w(sUtf), bFileExists );
	}
	int cmd = popup.TrackPopup( TPM_RETURNCMD|TPM_RIGHTBUTTON, pt, m_hWnd );
	if( cmd>=100 ) {
		if( GetAsyncKeyState(VK_CONTROL)&0x8000 )
			page.sci.GotoPos( vIncludes[cmd-100].pos );
		else {
			string str = ExtractFilePath( page.sFile ) + vIncludes[cmd - 100].second;
			recalcPath( str );
			FileOpen( str, false, getVectorIndex(m_vTabs, page, 1) );
		}
	}
}

void CEditor::onPopupStatusbar( int part, CTabPage& page )
{
	int x = m_statusbar.part2x(part);
	CPoint pt = CPoint(x,-1);
	m_statusbar.ClientToScreen(pt);
	switch( part )
	{
		case 0: onPopupGoto(page,pt); break;
		case 1: onPopupOverType(page,pt); break;
		case 2: onPopupLexers(page,pt); break;
		case 3: onPopupEncodings(page,pt); break;
		case 4: onPopupEOL(page,pt); break;
	}
}

void CEditor::onPopup( HWND hCtrl, int x, int y )
{
	int iTab = m_tabs.GetCurSel();
	if( iTab<0 || iTab>=(int)m_vTabs.size() )
		return;
	CTabPage& page = m_vTabs[iTab];

	CPoint pt(x,y);
	if( hCtrl==m_rebar.hWnd() )
	{
		if( m_cmbFunc.IsVisible() && m_cmbFunc.GetRect().in(pt) )
			onPopupFuncsSort( page, pt );
	} else
	if( hCtrl==m_statusbar.hWnd() )
	{
		m_statusbar.ScreenToClient(pt);
		int part = m_statusbar.x2part(pt.x);
		onPopupStatusbar( part, page );
	} else
	if( hCtrl==m_tabs.hWnd() )
	{
		m_tabs.ScreenToClient(pt);
		int iPopupTab = m_tabs.getItemIndexAt(pt);
		if( iPopupTab<0 )
			return;
		CRect r = m_tabs.GetItemRect(iPopupTab);
		pt = CPoint(r.left, r.bottom);
		m_tabs.ClientToScreen(pt);

		CTabPage& popupPage = m_vTabs[iPopupTab];
		if( GetAsyncKeyState(VK_SHIFT)&0x8000 )
			onPopupShellMenu( popupPage, pt );
		else
			onPopupTab( popupPage, iPopupTab, pt );
	} else
	if( hCtrl==page.sci.hWnd() )
	{
		if( GetAsyncKeyState(VK_SHIFT)&0x8000 )
			onPopupStyles( page, pt );
		else
		if( GetAsyncKeyState(VK_CONTROL)&0x8000 )
			onPopupIncludes( page, pt );
		else
			onPopupSci( page, pt );
	}
}

void CEditor::UpdateToolbar()
{
	CTabPage* pPage = NULL;
	int iTab = m_tabs.GetCurSel();
	if( iTab>=0 && iTab<(int)m_vTabs.size() )
		pPage = &m_vTabs[iTab];
	bool bSel = pPage ? pPage->sci.GetSelectionStart() != pPage->sci.GetSelectionEnd() : false;

	m_toolbar.Enable( ID_FILE_SAVE, pPage && pPage->sci.isModified() );
	m_toolbar.Enable( ID_FILE_SAVEAS, pPage!=0 );
	m_toolbar.Enable( ID_FILE_SAVEALL, pPage!=0 );

	m_toolbar.Enable( ID_EDIT_UNDO, pPage && pPage->sci.CanUndo() );
	m_toolbar.Enable( ID_EDIT_REDO, pPage && pPage->sci.CanRedo() );
	m_toolbar.Enable( ID_EDIT_CUT, bSel );
	m_toolbar.Enable( ID_EDIT_COPY, bSel );
	m_toolbar.Enable( ID_EDIT_COPYASHTML, bSel );
	m_toolbar.Enable( ID_EDIT_PASTE, pPage && pPage->sci.CanPaste() );
	m_toolbar.Enable( ID_EDIT_DELETE, pPage!=0 );

	m_toolbar.Enable( ID_EDIT_READONLY, pPage!=0 );
	m_toolbar.Check( ID_EDIT_READONLY, pPage && pPage->sci.IsReadOnly() );
	m_toolbar.Enable( ID_EDIT_FORMATXML, pPage!=0 );

	m_toolbar.Enable(ID_VIEW_WRAPLINES, pPage != 0);
	m_toolbar.Check(ID_VIEW_WRAPLINES, pPage && pPage->sci.getWrapLongLines());

	m_toolbar.Enable(ID_VIEW_EOL, pPage != 0);
	m_toolbar.Check(ID_VIEW_EOL, pPage && pPage->sci.GetViewEOL());
}

void CEditor::UpdateMenu( HMENU hMenu, CTabPage* pPage )
{
	if( !pPage ) {
		int iTab = m_tabs.GetCurSel();
		if( iTab>=0 && iTab<(int)m_vTabs.size() )
			pPage = &m_vTabs[iTab];
	}

	bool bSel = pPage ? pPage->sci.GetSelectionStart() != pPage->sci.GetSelectionEnd() : false;
	bool bReadOnly = pPage ? pPage->sci.IsReadOnly() : false;

	CMenu menu( hMenu );
	menu.Enable( ID_EDIT_UNDO, pPage && pPage->sci.CanUndo() );
	menu.Enable( ID_EDIT_REDO, pPage && pPage->sci.CanRedo() );
	menu.Enable( ID_EDIT_CUT, bSel );
	menu.Enable( ID_EDIT_COPY, bSel );
	menu.Enable( ID_EDIT_COPYASHTML, bSel );
	menu.Enable( ID_EDIT_COPYASHEX, bSel );
	menu.Enable( ID_EDIT_PASTE, pPage && pPage->sci.CanPaste() );
	menu.Enable( ID_EDIT_DELETE, pPage!=0 );
	menu.Enable( ID_EDIT_SELECTALL, pPage!=0 );
	menu.EnableCheck( ID_EDIT_READONLY, pPage!=0, pPage && pPage->sci.IsReadOnly() );
	menu.Enable( ID_EDIT_UPPERCASE, bSel );
	menu.Enable( ID_EDIT_LOWERCASE, bSel );
	menu.Enable( ID_EDIT_AUTOC, pPage!=0 );
	menu.Enable( ID_EDIT_CALLTIP, pPage!=0 );
	menu.Enable( ID_EDIT_SORT, pPage!=0 && !pPage->sci.IsReadOnly() );
	menu.Enable( ID_EDIT_FORMATINDENT, pPage!=0 && !pPage->sci.IsReadOnly() );
	menu.Enable( ID_EDIT_FORMATXML, pPage!=0 && !pPage->sci.IsReadOnly() );
	menu.Enable( ID_EDIT_WRAPLINES, pPage != 0 && !pPage->sci.IsReadOnly());
	menu.Enable( ID_EDIT_STRIPTRAILINGSPACES, pPage!=0 && !pPage->sci.IsReadOnly() );
    menu.Enable( ID_EDIT_VP, pPage!=0 );

	menu.Enable( ID_SEARCH_FIND, pPage!=0 );
	menu.Enable( ID_SEARCH_REPLACE, pPage!=0 );
	menu.Enable( ID_SEARCH_FINDFILE, pPage!=0 );
	menu.Enable( ID_SEARCH_REPLACEFILE, pPage!=0 );
	menu.Enable( ID_SEARCH_FINDNEXT, pPage!=0 );
	menu.Enable( ID_SEARCH_FINDPREV, pPage!=0 );
	menu.Enable( ID_SEARCH_GOTO, pPage!=0 );
	menu.Enable( ID_SEARCH_MATCHBRACE, pPage!=0 && pPage->sci.isBraceMatched() );
	menu.Enable( ID_SEARCH_MATCHFOLD, pPage!=0 && pPage->sci.isFoldMatched() );
	menu.Enable( ID_SEARCH_CLEARINDIC, pPage!=0 );

	menu.Enable( ID_FILE_SAVE, pPage && pPage->sci.isModified() );
	menu.Enable( ID_FILE_SAVEAS, pPage!=0 );
	menu.Enable( ID_FILE_CLOSE, pPage!=0 );
	menu.Enable( ID_FILE_SAVEALL, pPage!=0 );
	menu.Enable( ID_FILE_EXPORT2HTML, pPage!=0 );

	menu.Enable( ID_WINDOW_NEXT, pPage!=0 );
	menu.Enable( ID_WINDOW_PREV, pPage!=0 );
	menu.Enable( ID_WINDOW_CLOSE, pPage!=0 );
	menu.Enable( ID_WINDOW_CLOSE_GROUP, pPage!=0 );
	menu.Enable( ID_WINDOW_CLOSE_EXCEPT, pPage!=0 && m_tabs.GetItemCount()>=2 );
	menu.Enable( ID_WINDOW_CLOSE_TORIGHT, pPage!=0 && m_tabs.GetCurSel()+1<m_tabs.GetItemCount() );
	menu.Enable( ID_WINDOW_CLOSE_ALL, pPage!=0 );

	menu.EnableCheck( ID_VIEW_WS_NONE, pPage!=0, pPage && pPage->sci.isViewWSNone() );
	menu.EnableCheck( ID_VIEW_WS_PART, pPage!=0, pPage && pPage->sci.isViewWSPart() );
	menu.EnableCheck( ID_VIEW_WS_ALL, pPage!=0, pPage && pPage->sci.isViewWSAll() );
	menu.EnableCheck( ID_VIEW_EOL, pPage!=0, pPage && pPage->sci.GetViewEOL() );
	menu.EnableCheck( ID_VIEW_BRACE, pPage!=0, pPage && pPage->sci.isBraceHL() );
	menu.EnableCheck( ID_VIEW_NUMS, pPage!=0, pPage && pPage->sci.isViewNums() );
	menu.EnableCheck( ID_VIEW_INDENT, pPage!=0, pPage && pPage->sci.isViewIndentGuides() );
	menu.EnableCheck( ID_VIEW_LINEHL, pPage!=0, pPage && pPage->sci.GetCarretLineVisible() );
	menu.EnableCheck( ID_VIEW_XMLTAGS, pPage!=0, pPage && pPage->sci.isXmlMatchedTagHL() );
	menu.EnableCheck( ID_VIEW_TOKEN, pPage!=0, pPage && pPage->sci.isTokenHL() );
	menu.EnableCheck( ID_VIEW_WRAPLINES, pPage!=0, pPage && pPage->sci.getWrapLongLines() );

	menu.Enable( ID_VIEW_ZOOMIN, pPage!=0 );
	menu.Enable( ID_VIEW_ZOOMOUT, pPage!=0 );
	menu.Enable( ID_VIEW_ZOOMRESET, pPage!=0 && pPage->sci.GetZoom()!=0 );

	int foldExpanded = -1;
	if( pPage ) {
		int line = pPage->sci.GetCurrentLine();
		if( pPage->sci.IsFoldHeader(line) )
			foldExpanded = pPage->sci.GetFoldExpanded(line) ? 1 : 0;
	}
	menu.Enable( ID_FOLDING_COLLAPSE, pPage!=0 && foldExpanded==1 );
	menu.Enable( ID_FOLDING_COLLAPSECHILD, pPage!=0 && foldExpanded==1 );
	menu.Enable( ID_FOLDING_EXPAND, pPage!=0 && foldExpanded==0 );
	menu.Enable( ID_FOLDING_EXPANDCHILD, pPage!=0 && foldExpanded==0 );
	menu.Enable( ID_FOLDING_TOGGLE, pPage!=0 && foldExpanded>=0 );
	menu.Enable( ID_FOLDING_TOGGLECHILD, pPage!=0 && foldExpanded>=0 );
	menu.Enable( ID_FOLDING_COLLAPSEALL, pPage!=0 );
	menu.Enable( ID_FOLDING_EXPANDALL, pPage!=0 );
	menu.Enable( ID_FOLDING_TOGGLEALL, pPage!=0 );

	menu.EnableCheck( ID_TAB_PIN, pPage!=0, pPage && m_pin.isPinned(pPage->sFile) );
	menu.Enable( ID_TAB_MOVE2NEWAPE, pPage!=0 && !pPage->bNew && !pPage->sci.isModified() );
	menu.Enable( ID_TAB_OPEN2NEWAPE, pPage!=0 && !pPage->bNew && !pPage->sci.isModified() );

	menu.EnableCheck( ID_EOL_CRLF, pPage!=0 && !bReadOnly, pPage && pPage->sci.GetEOLMode()==SC_EOL_CRLF );
	menu.EnableCheck( ID_EOL_CR, pPage!=0 && !bReadOnly, pPage && pPage->sci.GetEOLMode()==SC_EOL_CR );
	menu.EnableCheck( ID_EOL_LF, pPage!=0 && !bReadOnly, pPage && pPage->sci.GetEOLMode()==SC_EOL_LF );

	bool b = pPage!=0 && !bReadOnly;
	CEncoding enc=encNone;
	if( pPage ) enc=pPage->sci.getEncoding();
	bool isEncUtf8 = enc==encUtf8 || enc==encUtf8NoBOM;
	menu.EnableCheck( ID_ENCODING_ANSI, b, enc==encAnsi );
	menu.EnableCheck( ID_ENCODING_UTF8, b, enc==encUtf8 );
	menu.EnableCheck( ID_ENCODING_UTF8_NO_BOM, b, enc==encUtf8NoBOM );
	menu.EnableCheck( ID_ENCODING_UTF16BE, b, enc==encUtf16BE );
	menu.EnableCheck( ID_ENCODING_UTF16LE, b, enc==encUtf16LE );
	menu.EnableCheck( ID_ENCODING_LATIN1, b, enc==encLatin1 );
	menu.EnableCheck( ID_ENCODING_LATIN2, b, enc==encLatin2 );
	menu.EnableCheck( ID_ENCODING_LATIN3, b, enc==encLatin3 );
	menu.EnableCheck( ID_ENCODING_ANSI_CYR, b, enc==encAnsiCyr );

	menu.EnableCheck( ID_OPTIONS_NOTONEINSTANCE, g_options.bOneInstanse, m_bNotOneInstance );
}

void CEditor::UpdateStatusbar()
{
	int iTab = m_tabs.GetCurSel();
	if( iTab<0 || iTab>=(int)m_vTabs.size() ) {
		for( int i=0; i<5; i++ )
			m_statusbar.SetText( i, "" );
		return;
	}
	CTabPage& page = m_vTabs[iTab];

	int len = page.sci.GetTextLength();
	int pos, line, col;
	page.sci.GetCurrentPosLineCol( pos, line, col );
	int fold = page.sci.FoldLevel(line);
	unsigned char ch = page.sci.GetCharAt(pos);
	string s;
	s	<< "Ln "<< line+1 <<"  Col "<< col+1 << "    "
		<< "Fold "<< fold <<"  Char " << Format("0x%02X",ch) << "    "
		<< "Pos "<< pos <<"  Size "<< len << "    "
		<< "Selection "<< page.sci.GetSelectionEnd()-page.sci.GetSelectionStart();
	m_statusbar.SetText( 0, s );
	m_statusbar.SetText( 1, page.sci.GetOverType() ? "OVR" : "INS" );

	const char* szLexer=0;
	for( CLexerInfo* pLex=g_lexers; pLex->iLexerID; pLex++ )
		if( pLex->iLexerID==page.sci.Lexer() )
		{
			szLexer = pLex->szLexerName;
			break;
		}
	m_statusbar.SetText( 2, szLexer );
	m_statusbar.SetText( 3, page.sci.getEncodingString().c_str() );
	m_statusbar.SetText( 4, page.sci.getEOLString().c_str() );
}

void CEditor::UpdateStatusText( const char* sz ) {
	m_statusbar.SetText( 5, sz );
}

void CEditor::UpdateStatusText( const wchar_t* wsz ) {
	m_statusbar.SetText( 5, wsz );
}

void CEditor::UpdateTitle() {
	wstring wsTitle;
	int iTab = m_tabs.GetCurSel();
	if( iTab>=0 )
		wsTitle = utf2w(m_vTabs[iTab].sFile) + L" - " + APE_TITLE;
	else
		wsTitle = APE_TITLE;
	SetWindowTextW(m_hWnd, wsTitle.c_str());
}

LRESULT CEditor::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch( msg ) {
	case WM_CLOSE:
		if( WindowCanCloseAll() )
			DestroyWindow(hWnd);
		return 0;

	case WM_DESTROY:
		onDestroy();
		PostQuitMessage(0);
		break;

	case WM_SIZE:
		onSize( LOWORD(lParam), HIWORD(lParam) );
		break;

	case WM_COMMAND:
		if( LOWORD(wParam) )
			onCommand( LOWORD(wParam), HIWORD(wParam) );
		break;

	case WM_NOTIFY:
		{
			auto* pNMHdr = (NMHDR*)lParam;
			if( pNMHdr->code==NM_CLICK && pNMHdr->hwndFrom==m_statusbar.hWnd() ) {
				int iTab = m_tabs.GetCurSel();
				if( iTab>=0 && iTab<(int)m_vTabs.size() ) {
					auto* p = (NMMOUSE*)lParam;
					onPopupStatusbar(p->dwItemSpec, m_vTabs[iTab] );
				}
				return TRUE;
			} else
			if( pNMHdr->code==TTN_GETDISPINFOW ) {
				if( pNMHdr->hwndFrom == m_tabs.GetToolTips() ) {
					unsigned id = pNMHdr->idFrom;
					if( id < m_vTabs.size() ) {
						auto* pNMTip = (NMTTDISPINFOW*)lParam;
						wstring ws;
						ws << utf2w(m_vTabs[id].sFile)
                          << "  " << vFileAttrs2Str(m_vTabs[id].sFile)
                          << "  " << vGetFileSize(m_vTabs[id].sFile)
                          << "  " << vGetFileTime(m_vTabs[id].sFile).Format("%d/%m/%Y %H:%M:%S");
						static wchar_t szTemp[1024];
						wcsncpy( szTemp, ws.c_str(), ws.size() );
						pNMTip->lpszText = szTemp;
					}
					return 0;
				}
			}
			if( pNMHdr->code==TTN_NEEDTEXTW || pNMHdr->code==TTN_NEEDTEXTA ) {
				if( 0<=pNMHdr->idFrom && pNMHdr->idFrom<=0xFFFF )
					HandleDialogTooltips( pNMHdr, GetMenu(hWnd) );
			} else
				onNotification( pNMHdr );
		}
		break;

	case WM_CONTEXTMENU:
		onPopup( (HWND)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
		break;

	case WM_COPYDATA:
		return onCopyData( (COPYDATASTRUCT*)lParam );

	case WM_TIMER:
		if( wParam==ID_CHECK_RELOAD ) {
			KillTimer( wParam );
			onCheckReload();
		} else
		if( wParam==ID_FILL_FUNC ) {
			KillTimer( wParam );
			int iTab = m_tabs.GetCurSel();
			if( iTab>=0 && iTab<(int)m_vTabs.size() ) {
				CTabPage& page = m_vTabs[iTab];
				FillFuncs(page,0);
				FillFuncCombo(page);
			}
		} else
		if( wParam==ID_TOKEN_HL ) {
			KillTimer( wParam );
			int iTab = m_tabs.GetCurSel();
			if( iTab>=0 && iTab<(int)m_vTabs.size() ) {
				CTabPage& page = m_vTabs[iTab];
				page.sci.DoTokenHL();
			}
		}
		break;

	case WM_ACTIVATE:
		if( wParam!=WA_INACTIVE ) {
			int iTab = m_tabs.GetCurSel();
			if( iTab>=0 )
				::SetFocus(m_vTabs[iTab].sci.hWnd() );
//			SetTimer( ID_CHECK_RELOAD, 100 );
			m_tabs.CloseBtnHover(-1,CPoint());
		}
		break;

	case WM_ACTIVATEAPP:
		if( wParam==TRUE ) {
			if( g_options.bCheckExternalModify )
				SetTimer( ID_CHECK_RELOAD, 100 );
		} else {
			if( g_options.bSaveOnDeactivate )
				FileSaveAll( true );
		}
		return 1;

	case WM_INITMENU:
		{
			HMENU hMenu = GetMenu(m_hWnd);
			HMENU hMRU = GetSubMenu(hMenu,1);
			HMENU hPin = GetSubMenu(hMenu,2);
			int c = GetMenuItemCount(hMenu);
			HMENU hLexers = GetSubMenu(hMenu,c-4);
			HMENU hTools = GetSubMenu(hMenu,c-3);
			int cnt = GetMenuItemCount(hTools);
			HMENU hLang = GetSubMenu(hTools,cnt-3);

			m_mru.UpdateMenu( hMRU );
			m_pin.UpdateMenu( hPin );
			UpdateLexersMenu( hLexers );
			g_lang.updateMenu( hLang );

			UpdateMenu(hMenu);
		}
		break;

	case WM_INITMENUPOPUP:
		m_bmpMenu.OnInitMenuPopup( (HMENU)wParam );
		break;

	case WM_MEASUREITEM:
		m_bmpMenu.OnMeasureItem( (MEASUREITEMSTRUCT*)lParam );
		break;

	case WM_DRAWITEM:
		if( wParam==m_tabs.m_id ) {
			unsigned index = ((DRAWITEMSTRUCT*)lParam)->itemID;
			bool bRO = false;
			bool bExist = false;
			if( index < m_vTabs.size() ) {
				bRO = m_vTabs[index].sci.IsReadOnly();
				bExist = m_vTabs[index].bNew || m_vTabs[index].bExist;
			}
			m_tabs.DrawItem( (DRAWITEMSTRUCT*)lParam, bRO, !bExist );
		} else
			m_bmpMenu.OnDrawItem( (DRAWITEMSTRUCT*)lParam );
		break;

	case WM_DROPFILES:
		DropFiles( (HDROP)wParam );
		break;

	case WM_TAB_CHANGING: onTabChanging(); break;
	case WM_TAB_CHANGED: onTabChanged(); break;
	case WM_TAB_REORDER: ReorderTabs(); break;
	case WM_TAB_MOVE2NEWAPE: onMove2NewApe(true); break;
	case WM_TAB_CLOSE: 
		WindowClose_int(lParam); 
		SwitchToTab(m_tabs.GetCurSel()); 
		break;

	default:
		return DefWindowProcW(hWnd, msg, wParam, lParam); // SetWindowTextW!
	}
	return 0;
}

LRESULT CALLBACK CEditor::WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	auto* pEditor = (CEditor*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if( pEditor )
		return pEditor->WndProc(hWnd, msg, wParam, lParam);
	else
		return DefWindowProcW(hWnd, msg, wParam, lParam); // SetWindowTextW!
}

/////////////////////////////////////////////////////////////////////

bool CEditor::FileOpen( const string& sFile, bool bNew, int idx, int top, int pos )
{
	ASSERT(m_tabs.GetItemCount() == m_vTabs.size() );

	if( !bNew )
	{
		if( !IsFileExists(sFile) ) {
			UpdateStatusText( "Can't open: "+sFile );
			return false;
		}
		m_mru.Add(sFile);
		for( size_t i=0; i < m_vTabs.size(); ++i )
			if( EqualsFilePath(m_vTabs[i].sFile, sFile) ) {
				SwitchToTab(i);
				return true;
			}
	}

	if( idx<0 || idx>=m_tabs.GetItemCount() )
		idx=-1;

	CWaitCursor cur;
	if( idx==-1 )
		m_vTabs.emplace_back( );
	else
		m_vTabs.insert(m_vTabs.begin() + idx, CTabPage() );
	CTabPage& page = idx==-1 ? m_vTabs.back() : m_vTabs[idx];
	page.sFile = sFile;
	page.bNew = bNew;
	page.sci.Create( m_hWnd );
	int index;
	if( idx==-1 )
		index = m_tabs.InsertItem("", m_ilTabs.getFileTypeIndex(sFile) ); // utf2w(ExtractFileName(sFile))
	else {
		// insert new tab to the end
		m_tabs.InsertItem( "", -1 );
		// shift tabs from current
		wstrbuf sBuf(MAX_PATH);
		TCITEMW it{};
		it.mask = TCIF_IMAGE | TCIF_TEXT | TCIF_PARAM;
		it.pszText = sBuf.buf();
		it.cchTextMax = sBuf.size();
		for( int i=m_tabs.GetItemCount()-1; i>idx; --i ) {
			m_tabs.GetItem( i-1, &it );
			m_tabs.SetItem( i, &it );
		}
		// update current tab
		m_tabs.UpdateItem(idx, "", m_ilTabs.getFileTypeIndex(sFile) ); // utf2w(ExtractFileName(sFile))
		index = idx;
	}
	if( index<0 )
		return false;
	if( bNew ) {
		page.sci.SetLexer( LEX_TXT );
		SetupLexerStyles( page.sci );
	} else {
		if( !page.loadFromFile() ) {
			WindowClose_int(index);
			return false;
		}
		page.sci.SetLexer( DetectLexer(sFile) );
		SetupLexerStyles( page.sci );
		FillFuncs( page, 0 );
	}
	SetupIndicators( page.sci );
	SwitchToTab( index, m_tabs.GetItemCount()==1 );
	if ( top > 0 )
		page.sci.SetFirstVisibleLine( top );
	if ( pos > 0 )
		page.sci.GotoPos( pos );
	if( ExtractFileExt(sFile)=="log" && top==-1 && pos==-1 ) {
		int p = page.sci.GetTextLength();
		page.sci.GotoPos( p );
		page.sci.EnsureVisible(p,p);
	}
	return true;
}

void CEditor::FileNew() {
	FileOpen( "(Untitled)", true );
}

void CEditor::FileOpen() {
	string sFile;
	if( dlgOpenFile(m_hWnd,sFile) )
		FileOpen( sFile, false );
}

void CEditor::FileSave_int( int iTab, bool bSaveAs )
{
	CTabPage& page = m_vTabs[iTab];
	if( page.bNew || bSaveAs )
	{
		if( !dlgSaveFile(m_hWnd,page.sFile) )
			return;
		m_mru.Add(page.sFile);
		m_tabs.UpdateItem(iTab, ExtractFileName(page.sFile), m_ilTabs.getFileTypeIndex(page.sFile) );
		page.sci.SetLexer( DetectLexer(page.sFile) );
		SetupLexerStyles( page.sci );
		page.sci.ReStyle();

		FillFuncs(page,0);
		if( iTab==m_tabs.GetCurSel() )
		{
			FillFuncCombo(page);
			ShowFuncCombo(page);
		}
	}
	page.bNew = false;
	page.bExist = true;
	page.saveToFile();
}

void CEditor::FileSave( bool bSaveAs )
{
	ASSERT(m_tabs.GetItemCount() == m_vTabs.size() );

	int iTab = m_tabs.GetCurSel();
	if( iTab<0 || iTab>=(int)m_vTabs.size() )
		return;

	FileSave_int( iTab, bSaveAs );
	UpdateToolbar();
	if( bSaveAs )
		UpdateTitle();
}

void CEditor::FileSaveAll( bool bDeactivate )
{
	for( unsigned i=0; i < m_vTabs.size(); i++ )
	{
		if( bDeactivate && (m_vTabs[i].bNew || m_vTabs[i].sci.IsReadOnly()) )
			continue;
		if( m_vTabs[i].sci.isModified() )
		{
			FileSave_int(i,false);
			UpdateTab(i);
		}
	}
	UpdateToolbar();
}

/////////////////////////////////////////////////////////////////////

bool CEditor::WindowCanClose_int(int iTab)
{
	CTabPage& tabPage = m_vTabs[iTab];
	if( !tabPage.sci.isModified() )
		return true;
	string msg;
	msg << "Save file " << tabPage.sFile.c_str() << "?";
	int ret = MessageBox( msg, "Save", MB_YESNOCANCEL );
	if( ret==IDCANCEL )
		return false;
	if( ret==IDYES )
	{
		if( !tabPage.saveToFile() )
			return false;
		UpdateTab(iTab);
	}
	return true;
}

bool CEditor::WindowClose_int(int iTab)
{
	if( !WindowCanClose_int(iTab) )
		return false;
	CTabPage& tabPage = m_vTabs[iTab];
	tabPage.sci.Destroy();
	m_vTabs.erase(m_vTabs.begin() + iTab, m_vTabs.begin() + iTab + 1 );
	m_tabs.DeleteItem( iTab );
	return true;
}

bool CEditor::WindowClose_int(const string& sFile)
{
	for( unsigned i=0; i < m_vTabs.size(); i++ )
		if( m_vTabs[i].sFile == sFile )
			return WindowClose_int(i);
	return false;
}

void CEditor::WindowClose()
{
	int iTab = m_tabs.GetCurSel();
	if( iTab<0 || iTab>=(int)m_vTabs.size() )
		return;

	if( !WindowClose_int(iTab) )
		return;

	SwitchToTab( iTab-1 );
}

void CEditor::WindowCloseGroup()
{
	int iTab = m_tabs.GetCurSel();
	if( iTab<0 || iTab>=(int)m_vTabs.size() )
		return;

	string sFile = m_vTabs[iTab].sFile;
	string sFile2 = doAlternate(sFile);
	if( !WindowClose_int(sFile) )
		return;
	if( !sFile2.empty() )
		WindowClose_int(sFile2);

	SwitchToTab( iTab-1 );
}

void CEditor::WindowCloseExcept()
{
	int iTab = m_tabs.GetCurSel();
	for( int i=0; m_vTabs.size() > 1; )
	{
		int iTab = m_tabs.GetCurSel();
		if( i==iTab ) { i++; continue; }

		if( !WindowClose_int(i) )
			break;
	}

	SwitchToTab( iTab-1 );
}

void CEditor::WindowCloseToRight()
{
	int idx = m_tabs.GetCurSel();
	while( (int)m_vTabs.size() > idx + 1 )
		if( !WindowClose_int(m_vTabs.size() - 1) )
			break;

	// no need to switch tab
}

void CEditor::WindowCloseAll()
{
	while( m_vTabs.size() > 0 )
		if( !WindowClose_int(0) )
			break;
	SwitchToTab( m_tabs.GetCurSel() );
}

bool isModified(CTabPage& page) { return page.sci.isModified(); }

bool CEditor::WindowCanCloseAll()
{
	bool bOk = true;
	if( any_of(m_vTabs.begin(), m_vTabs.end(), isModified) ) {
		CCheckListDlg dlg(m_vTabs, m_ilTabs, CCheckListDlg::modeSaveChanges);
		bOk = dlg.doModal(m_hWnd);
		for( size_t i=0; i < m_vTabs.size(); ++i )
			if( dlg.isChanged(m_vTabs[i].sFile) )
				UpdateTab(i);
	}
	return bOk;
}

/////////////////////////////////////////////////////////////////////

void CEditor::onTabChanging()
{
	m_tabs.iPrevSel = m_tabs.GetCurSel();
}

void CEditor::onTabChanged()
{
	int index = m_tabs.GetCurSel();
	int indexPrev = m_tabs.iPrevSel;
	m_tabs.iPrevSel = -1;
	if( index==indexPrev )
		return;
	if( indexPrev>=0 && indexPrev<(int)m_vTabs.size() )
		::ShowWindow(m_vTabs[indexPrev].sci.hWnd(), SW_HIDE );
	if( index>=0 && index<(int)m_vTabs.size() )
	{
		::ShowWindow(m_vTabs[index].sci.hWnd(), SW_SHOW );
		::SetFocus(m_vTabs[index].sci.hWnd() );
	}
	ResizeScintilla();
	UpdateStatusbar();
	UpdateStatusText("");
	UpdateToolbar();
	UpdateTitle();

	CTabPage& page = m_vTabs[index];
	ShowFuncCombo( page );
	if( m_cmbFunc.IsVisible() )
		FillFuncCombo( page );
}

void CEditor::SwitchToTab( int iTab, bool bReset )
{
	m_tabs.SetVisible( m_tabs.GetItemCount()>0 );
	m_pnl.SetVisible( m_tabs.GetItemCount()==0 );
	if( m_tabs.GetItemCount()==0 )
	{
		UpdateStatusbar();
		UpdateStatusText("");
		UpdateToolbar();
		UpdateTitle();
		m_cmbFunc.Clear();
		m_cmbFunc.SetVisible( false );
		return;
	}
	if( iTab<0 )
		iTab = 0;
	if( iTab>=m_tabs.GetItemCount() )
		iTab = m_tabs.GetItemCount()-1;
	onTabChanging();
	if( bReset )
		m_tabs.iPrevSel = -1;
	m_tabs.SetCurSel(iTab);
	onTabChanged();
}

/////////////////////////////////////////////////////////////////////

bool CEditor::onCopyData( COPYDATASTRUCT* pCDS )
{
	if( !pCDS->lpData || !pCDS->cbData )
		return false;
	if( pCDS->cbData==sizeof(DWORD) ) {
		return *(DWORD*)pCDS->lpData==INSTANCE_CODE && !m_bNotOneInstance;
	} else {
		string s((const char*)pCDS->lpData, pCDS->cbData);
		vector<string> vFiles;
		split(s, vFiles, 0x9);
		for( const auto& sFile: vFiles )
			FileOpen(sFile, false);
		return true;
	}
}

bool isExtModified(CTabPage& page)
{
	if( page.bNew || !page.IsFileExists() )
		return false;
	CTime tm = vGetFileTime( page.sFile );
	int64_t size = vGetFileSize( page.sFile );
	return page.tm!=tm || page.size!=size;
}

void CEditor::onCheckReload()
{
	if( !g_options.bCheckExternalModify )
		return;
	static bool bIn = false;
	if( bIn ) return;
	bIn = true;

	if( any_of(m_vTabs.begin(), m_vTabs.end(), isExtModified) ) {
		CCheckListDlg dlg(m_vTabs, m_ilTabs, CCheckListDlg::modeReloadChanges);
		dlg.doModal(m_hWnd);
		for( size_t i=0; i < m_vTabs.size(); ++i ) {
			if( dlg.isChanged(m_vTabs[i].sFile) ) {
				UpdateTab(i);
				FillFuncs(m_vTabs[i] );
				if( i==m_tabs.GetCurSel() )
					FillFuncCombo(m_vTabs[i] );
			}
		}
	}
	m_tabs.Invalidate(); // repaint if file is deleted

	bIn = false;
}

void CEditor::onGoto(CTabPage& page, bool bGotoLine) {
	CGotoDlg dlg;
    dlg.iCurLine = page.sci.GetCurrentLine();
    dlg.iLastLine = page.sci.GetLineCount();
    dlg.iCurPos = page.sci.GetCurrentPos();
    dlg.iLastPos = page.sci.GetTextLength();
    dlg.bGotoLine = bGotoLine;
	if( dlg.doModal(m_hWnd) ) {
        if( dlg.bGotoLine )
            page.sci.GotoPos( page.sci.PositionFromLine(dlg.iGoto) );
        else
            page.sci.GotoPos( dlg.iGoto );
    }
}

void CEditor::onColors() {
	CLexerStyle* pStyles=nullptr;
	int iTab = m_tabs.GetCurSel();
	if( iTab>=0 )
		pStyles = GetLexerStyles(m_vTabs[iTab].sci );

	CColorsDlg dlg(pStyles);
	if( dlg.doModal(m_hWnd) )
		for(auto& tabPage : m_vTabs)
			SetupLexerStyles(tabPage.sci );
}

void CEditor::onIndicators() {
	CIndicatorsDlg dlg;
	if( dlg.doModal(m_hWnd) )
		for(auto& tabPage : m_vTabs)
			SetupIndicators(tabPage.sci );
}

void CEditor::onOptions() {
	COptionsDlg dlg;
	if( dlg.doModal(m_hWnd) )
		m_tabs.applyOptions();
}

void CEditor::onSort() {
	int iTab = m_tabs.GetCurSel();
	if( iTab<0 ) return;
	CSciWrapper& sci = m_vTabs[iTab].sci;
    if( sci.IsReadOnly() ) return;

	CSortDlg(sci).doModal(m_hWnd);
}

void CEditor::DropFiles(HDROP hdrop) {
	if( hdrop ) {
		wstrbuf sBuf(MAX_PATH);
		UINT cnt = ::DragQueryFileW(hdrop, 0xffffffff, nullptr, 0);
		for( int i = 0; i < cnt; ++i ) {
			UINT len = ::DragQueryFileW(hdrop, i, sBuf.buf(), sBuf.size());
            wstring sFile = sBuf.str(len); // len without ending zero
			if( !FileOpen(w2utf(sFile), false) )
				break;
		}
		::DragFinish(hdrop);
		if( ::IsIconic(m_hWnd) )
			::ShowWindow(m_hWnd, SW_RESTORE);
		::SetForegroundWindow(m_hWnd);
	}
}

void CEditor::ViewAlwaysOnTop()
{
	CMenu menu( GetMenu(m_hWnd) );
	bool bAoT = !menu.isChecked(ID_VIEW_ALWAYSONTOP);
	menu.Check(ID_VIEW_ALWAYSONTOP,bAoT);
	SetWindowPos(m_hWnd, bAoT?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
}

void CEditor::ViewFullScreen()
{
	CMenu menu( GetMenu(m_hWnd) );
	bool bFS = !menu.isChecked(ID_VIEW_FULLSCREEN);
	menu.Check(ID_VIEW_FULLSCREEN,bFS);
	SetFullScreen( bFS );
}

void CEditor::SetFullScreen(bool bOn)
{
	bool isFullScreen = (GetStyle() & WS_CAPTION) == 0;
	if( bOn==isFullScreen )
		return;

	if( bOn ) {
		ModifyStyle( WS_CAPTION|WS_THICKFRAME|WS_MAXIMIZEBOX, NULL );
		m_bWasZoomed=false;
		if( IsZoomed(m_hWnd) )
		{
			::ShowWindow(m_hWnd,SW_RESTORE);
			m_bWasZoomed=true;
		}
		::ShowWindow(m_hWnd,SW_MAXIMIZE);
	} else {
		ModifyStyle( NULL, WS_CAPTION|WS_THICKFRAME|WS_MAXIMIZEBOX );
		if( m_bWasZoomed )
			::ShowWindow(m_hWnd,SW_MAXIMIZE);
		else
			::ShowWindow(m_hWnd,SW_RESTORE);
	}
	::SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
		SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_FRAMECHANGED|SWP_DRAWFRAME );
}

/////////////////////////////////////////////////////////////////////
// todo: GetWordFromPosition & GetIdentFromPosition

void parseJsAutoComplete( CSciWrapper& sci, int pos, string& str, string& sSelect )
{
	sSelect = sci.GetWordFromPosition(pos,true,false);
	int pos1 = sci.PositionFromLine( sci.LineFromPosition(pos) );
	int pos2 = pos - sSelect.length();
	string sObject;
	bool bDot = false;
	if( pos1>=0 && pos2>0 && pos2>pos1 )
		if( sci.GetCharAt(pos2-1)=='.' ) {
			int p;
			sObject = sci.GetIdentFromPosition(pos2-1,p);
			char ch = sci.GetCharAt(p-1);
			bDot = ch=='.';
		}
	jsFix4JQuery( sObject, bDot );
	if( sObject.empty() )
		jsGetRootList( str );
	else
		jsGetObjectList( sObject, str );
}

void parseJsCallTip( CSciWrapper& sci, int pos, string& sObject, string& sMember, string& sPrm, string& sDesc )
{
	sMember = sci.GetWordFromPosition(pos);
	int pos1 = sci.PositionFromLine( sci.LineFromPosition(pos) );
	int pos2 = pos - sci.GetWordFromPosition(pos,true,false).length();
	bool bDot = false;
	if( pos1>=0 && pos2 && pos2>pos1 )
		if( sci.GetCharAt(pos2-1)=='.' ) {
			int p;
			sObject = sci.GetIdentFromPosition(pos2-1,p);
			char ch = sci.GetCharAt(p-1);
			bDot = ch=='.';
		}
	jsFix4JQuery( sObject, bDot );
	if( sObject.empty() )
		jsGetRootParams( sMember, sPrm, sDesc );
	else
		jsGetObjectParams( sObject, sMember, sPrm, sDesc );
}

void CEditor::onAutoComplete( CSciWrapper& sci )
{
	int pos = sci.GetCurrentPos();
	string str, sSelect;
	if( isJSCode(sci) )
		parseJsAutoComplete(sci,pos,str,sSelect);
	else
	if( isCSSAttr(sci) )
	{
		sSelect = sci.GetWordFromPosition(pos,true,false,"-");
		str = cssGetKeywordList();
	} else
	if( isCSSValue(sci) )
	{
		sSelect = sci.GetWordFromPosition(pos,true,false,"-");
		string sKeyword = cssGetKeyword(sci);
		str = cssGetValueList(sKeyword);
	} else
	if( sci.Lexer()==LEX_NSIS )
	{
		if( pos && sci.GetCharAt(pos-1)=='.' )
			return;
		initNSIS();
		sSelect = sci.GetWordFromPosition(pos,true,false);
		str = g_sApiNSIS;
	}

	sci.AutoComplete( str, sSelect );
}

void CEditor::onCallTip( CSciWrapper& sci )
{
	sci.CalltipCancel();

	int pos = sci.GetCurrentPos();
	string str;
	int start=0, end=0;

	if( isJSCode(sci) )
	{
		string sObject, sMember, sPrm, sDesc;
		parseJsCallTip( sci, pos, sObject, sMember, sPrm, sDesc );
		if( sPrm.empty() && sDesc.empty() ) {
			sci.positionCalltipBraces(pos);
			parseJsCallTip( sci, pos, sObject, sMember, sPrm, sDesc );
		}
		if( sPrm.empty() && sDesc.empty() ) return;
		if( !sObject.empty() )
			str << sObject << '.';
		str << sMember << ' ';
		start = str.length();
		str << sPrm;
		end = str.length();
		if( !sDesc.empty() )
			str << "\n\n" << sDesc;
		if( sPrm.length()>=2 )
		{
			if( strchr("([{",sPrm[0]) ) start++;
			if( strchr("]})",sPrm[sPrm.length()-1]) ) end--;
		}
	} else
	if( isCSSAttr(sci) )
	{
		string sKeyword = sci.GetWordFromPosition(pos,"-");
		str = cssGetKeywordDesc(sKeyword);
	} else
	if( isCSSValue(sci) )
	{
		string sValue = sci.GetWordFromPosition(pos,"-");
		string sKeyword = cssGetKeyword(sci);
		str = cssGetValueDesc(sKeyword,sValue);
	} else
	if( sci.Lexer()==LEX_NSIS )
	{
		string s = sci.GetWordFromPosition(pos);
		if( s.empty() ) return;
		CMapNSIS::iterator it = g_mapNSIS.find(s);
		if( it==g_mapNSIS.end() ) return;
		str << it->first << ' ';
		start = str.length();
		str << it->second.sParams;
		end = str.length();
		if( !it->second.sDesc.empty() )
			str << "\n\n" << it->second.sDesc;
	}

	if( str.empty() ) return;
	sci.CalltipShow( pos, str );
	sci.CalltipSetHlt( start, end );
}

///////////////////////////////////////////////////////////////////////////////

int ScrollFuncBraces( CSciWrapper& sci, int end, int pos, int stySymbol)
{
	int level=0;
	for( ; pos<end; pos++ )
		if( sci.GetStyleAt(pos)==stySymbol && sci.GetCharAt(pos)=='{' )
		{
			level++;
			pos++;
			break;
		}
	for( ; level && pos<end; pos++ )
		if( sci.GetStyleAt(pos)==stySymbol )
			switch( sci.GetCharAt(pos) )
			{
				case '{': level++; break;
				case '}': level--; break;
			}
	return pos;
}

void CEditor::fillFuncs_int( CTabPage& page, int start, int end )
{
	if( start<0 || end<0 || start>=end ) return;
	CSciWrapper& sci = page.sci;

	int styComment = 2;
	int stySymbol = 3;
	if( sci.Lexer()==LEX_XHTML )
	{
		styComment = 42;
		stySymbol = 43;
	}

	boost::regex expr( "function[ \\t]+([a-zA-Z0-9_#$]+)[ \\t]*(\\([^)]*\\))|"
			"([a-zA-Z0-9_#$.]+)[ \\t]*=[ \\t]*function[ \\t]*(\\([^)]*\\))|"
			"([a-zA-Z0-9_#$]+)[ \\t]*:[ \\t]*function[ \\t]*(\\([^)]*\\))" );
	while( true )
	{
		boost::match_results<CSciIterator> what;
		CSciIterator itA(&sci,start), itB(&sci,end);
		if( !boost::regex_search(itA, itB, what, expr, boost::match_default ) )
			break;
		if( what[0].first.getStyleAt()==styComment )
		{
			start = what[0].second.getPos();
		} else {
			int posA = what[0].first.getPos();
			int posB = what[0].second.getPos();
			string sName = GetTextRange(what[1]);
			string sPrms = GetTextRange(what[2]);
			int posName = what[1].first.getPos(); // jump to function name
			if( sName.empty() && sPrms.empty() )
			{
				sName = GetTextRange(what[3]);
				sPrms = GetTextRange(what[4]);
				posName = what[3].first.getPos();
			}
			if( sName.empty() && sPrms.empty() )
			{
				sName = GetTextRange(what[5]);
				sPrms = GetTextRange(what[6]);
				posName = what[5].first.getPos();
			}

			int posStart = posA;
			int posEnd = ScrollFuncBraces(sci,end,posB,stySymbol);
			int lineStart = sci.LineFromPosition(posStart);
			int lineEnd = sci.LineFromPosition(posEnd);
			page.vFunc.push_back( CFuncInfo(sName,sPrms,lineStart,lineEnd,posStart,posEnd,posName) );
			start = posEnd;
		}
	}
}

void CEditor::FillFuncs( CTabPage& page, int posFrom )
{
	CSciWrapper& sci = page.sci;
	if( sci.Lexer()!=LEX_XHTML && sci.Lexer()!=LEX_JS )
		return;
	if( sci.GetTextLength()>1000*1000 )
		return;
	posFrom = 0;
/*	if( posFrom )
	{
		posFrom = sci.PositionFromLine(sci.LineFromPosition(posFrom)-1);
		for( unsigned i=0; i<page.vFunc.size(); i++ ) // !!! use iterator
			if( page.vFunc[i].after(posFrom) )
			{
				page.vFunc.erase( page.vFunc.begin()+i, page.vFunc.end() );
				break;
			}
	} else*/
		page.vFunc.clear();
	sci.ColorizeTo(-1);
	int len = sci.GetTextLength();
	if( sci.Lexer()==LEX_JS )
		fillFuncs_int( page, posFrom, len );
	else {
		int start = -1;
		bool bJS=false;
		if( posFrom>0 )
		{
			int sty = sci.GetStyleAt(posFrom);
			if( sty>=40 && sty<50 )
			{
				start = posFrom;
				bJS = true;
			}
		}
		for( int i=posFrom; i<len; i++ )
		{
			int sty = sci.GetStyleAt(i);
			if( bJS )
			{
				if( sty<40 || sty>=50 )
				{
					fillFuncs_int( page, start, i-1 );
					bJS = false;
				}
			} else {
				if( sty>=40 && sty<50 )
				{
					start = i;
					bJS = true;
				}
			}
		}
	}
	if( m_bSortFuncsByName )
		page.vFunc.sortByName();
}

void CEditor::FillFuncCombo(CTabPage& page)
{
	m_cmbFunc.Clear();
	for( unsigned i=0; i<page.vFunc.size(); i++ )
		m_cmbFunc.AddString( page.vFunc[i].name + page.vFunc[i].params, i );
	UpdateFuncCombo(page);
}

void CEditor::ShowFuncCombo(CTabPage& page)
{
	m_cmbFunc.SetVisible( (page.sci.Lexer()==LEX_JS || page.sci.Lexer()==LEX_XHTML) && page.sci.GetTextLength()<1000*1000 );
}

void CEditor::UpdateFuncCombo(CTabPage& page)
{
	CSciWrapper& sci = page.sci;
	if( sci.Lexer()!=LEX_XHTML && sci.Lexer()!=LEX_JS )
		return;
	int pos = sci.GetCurrentPos();
	for( unsigned i=0; i<page.vFunc.size(); i++ )
		if( page.vFunc[i].in(pos) )
		{
			m_cmbFunc.SetupComboEx(i);
			return;
		}
	m_cmbFunc.SetCurSel(-1);
}

void CEditor::onFuncComboChange(CTabPage& page)
{
	int idx = m_cmbFunc.GetComboEx();
	if( idx<0 || idx>=(int)page.vFunc.size() )
		return;
	CSciWrapper& sci = page.sci;
	const CFuncInfo& func = page.vFunc[idx];
	sci.LineScroll( 0, func.lineStart-sci.LinesOnScreen()/2-sci.GetFirstVisibleLine() );
	sci.GotoPos( func.posName );
	sci.EnsureVisible( func.posName, func.posName );
	::SetFocus( sci.hWnd() );
}

void CEditor::onFuncJump(CTabPage& page)
{
	string s = page.sci.GetWordFromPosition(page.sci.GetCurrentPos());
	if( s.empty() )
		return;
	for( unsigned i=0; i<page.vFunc.size(); i++ )
		if( page.vFunc[i].name==s )
		{
			m_cmbFunc.SetupComboEx(i);
			onFuncComboChange(page);
			return;
		}
}

void CEditor::ReorderTabs()
{
	vector<CTabPage> vPages;
	for( int i=0, cnt=m_tabs.GetItemCount(); i<cnt; i++ ) {
		int idx = m_tabs.GetItemParam(i);
		if( idx<0 || idx>=(int)m_vTabs.size() )
			MessageBox(itoa(idx), "Error", MB_OK );
		vPages.push_back(m_vTabs[idx] );
	}
    m_vTabs = vPages;
}
