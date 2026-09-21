#pragma once

#include <windows.h>
#include <string>
#include <map>
#include <vector>

class CLang {
	int m_cmdFirst, m_cmdLast;
    std::vector<std::string> m_v;	// list of languages
    std::map<std::string, std::string> m_map;
	int m_lang;
public:
	CLang() { m_lang=-1; }
	void init( int cmdFirst, int cmdLast );
	void setLang( int lang );
    std::string translateStr( const std::string& s );		// utf-8
    std::wstring translateStr( const std::wstring& s );	// unicode
	void translateDlg( HWND hDlg );
	void translateMenu( HMENU hMenu );
	void translateMenu( HWND hWnd ) { translateMenu(GetMenu(hWnd)); DrawMenuBar(hWnd); }
	void updateMenu( HMENU hMenu );
	int langFromCmd(int cmd) { return cmd-m_cmdFirst-1; }
};

extern CLang g_lang;

inline std::string tr( const std::string& s ) { return g_lang.translateStr(s); }		// utf8

inline std::wstring tr( const std::wstring& ws ) { return g_lang.translateStr(ws); }	// unicode