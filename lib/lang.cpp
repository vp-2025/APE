#include "lang.h"
#include "shared/platform.h"
#include "shared/win.h" // GetWindowTextStrW & SetWindowTextStrW
#include "shared/str.h"
using namespace std;

CLang g_lang;

void CLang::init( int cmdFirst, int cmdLast )
{
	m_cmdFirst=cmdFirst; 
	m_cmdLast=cmdLast; 
}

void CLang::setLang( int lang )
{
	if( lang==-1 )
	{
		switch( PRIMARYLANGID(GetUserDefaultLCID()) )
		{
		case LANG_ENGLISH:	lang=0; break;
		case LANG_RUSSIAN:	lang=1; break;
		case LANG_GERMAN:	lang=2; break;
		default: lang=0; break;
		}
	}
	if( m_lang==lang ) return;
	m_lang = lang;

	m_map.clear();
	m_v.clear();

	string sData;
	if( !LoadTextFromFile(GetExePath()+"apeLng.txt",sData) )
		sData = LoadTextFromResource("TXT_APE_LNG");

	vector<string> vData;
	split( sData, vData, '\n' );
	for( unsigned i=0; i<vData.size(); i++ )
	{
		if( vData[i][0]==';' || vData[i][0]=='#' ) 
			continue; // skip comment line
		vector<string> v;
		split( vData[i], v, '\t' );
		if( v.empty() ) continue;
		if( m_v.empty() )
		{
			m_v = v;
			if( !m_lang ) break; // english
		} else
		if( m_lang<(int)v.size() )
			m_map[v[0]] = v[m_lang];
	}
}

string CLang::translateStr( const string& s )
{
	if( !m_lang ) return s; // english
	string sTr = m_map[s];
	if( sTr.empty() ) sTr = s;
	return sTr;
}

wstring CLang::translateStr( const wstring& s )
{
	if( !m_lang ) return s; // english
	wstring sTr = utf2w(m_map[w2utf(s)]);
	if( sTr.empty() ) sTr = s;
	return sTr;
}

void CLang::translateDlg( HWND hDlg ) {
	for( HWND hCtrl=hDlg; hCtrl!=nullptr; hCtrl=GetWindow(hCtrl,hCtrl==hDlg ? GW_CHILD:GW_HWNDNEXT) ) {
		wstring s = GetWindowTextStrW( hCtrl );
		if( s.empty() ) continue;
		wstring sTr = translateStr( s );
		if( sTr!=s )
		    SetWindowTextStr( hCtrl, sTr );
	}
}

void CLang::translateMenu( HMENU hMenu )
{
	int cnt = GetMenuItemCount(hMenu);
	for( int i=0; i<cnt; i++ )
	{
		HMENU hM = GetSubMenu(hMenu,i);
		if( hM )
			translateMenu( hM );
		UINT cmd = GetMenuItemID( hMenu, i );
		wstring sTr, s = GetMenuTextW(hMenu, i, MF_BYPOSITION);
        if( s.empty() ) continue;
		size_t p = s.find('\t');
		if( p!=string::npos )
			sTr = translateStr(s.substr(0,p)) + s.substr(p);
		else
			sTr = translateStr(s);
		if( sTr==s ) continue;
		ModifyMenuW( hMenu, i, MF_BYPOSITION|MF_STRING, cmd, sTr.c_str() );
	}
}

void CLang::updateMenu( HMENU hMenu )
{
	for( int i=m_cmdFirst; i<=m_cmdLast; i++ )
		DeleteMenu( hMenu, i, MF_BYCOMMAND );
	for( unsigned i=0; i<m_v.size(); i++ )
	{
		InsertMenuW( hMenu, i, MF_BYPOSITION|MF_STRING, m_cmdFirst+i+1, utf2w(m_v[i]).c_str() );
		if( m_lang==i )
			CheckMenuItem( hMenu, m_cmdFirst+i+1, MF_CHECKED );
	}
	InsertMenu( hMenu, 0, MF_BYPOSITION|MF_STRING, m_cmdFirst, "Detect" );
	InsertMenu( hMenu, 1, MF_BYPOSITION|MF_SEPARATOR, m_cmdLast, 0 );
}