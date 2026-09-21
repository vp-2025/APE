#include "versionInfo.h"
#include <windows.h>
#include "shared/platform.h"
#pragma comment(lib,"Version.lib")
using namespace std;

CVersionInfo::CVersionInfo( string sFileName ) {
	if( sFileName.empty() )
		sFileName = GetExeFileName();
	DWORD dwSize = GetFileVersionInfoSizeW( utf2w(sFileName).c_str(), nullptr );
	if( dwSize ) {
		m_sBuf.init(dwSize);
		if( GetFileVersionInfo( sFileName.c_str(), 0, m_sBuf.size(), m_sBuf.buf() ) ) {
			struct LANGANDCODEPAGE {
			  WORD wLanguage;
			  WORD wCodePage;
			} *lpTranslate;
			UINT cbTranslate;

			// Read the list of languages and code pages.
			VerQueryValueA( m_sBuf.buf(), "\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate );
			if( cbTranslate>0 ) {
				string sLang = Format("%04x", lpTranslate[0].wLanguage);
				string sCP = Format("%04x", lpTranslate[0].wCodePage);
				m_strInfo << "\\StringFileInfo\\" << sLang << sCP << "\\";
			}
		}
	}
}

string CVersionInfo::getInfoString( LPCTSTR sz) {
	if( m_strInfo.empty() )
		return {};

	string strBlock = m_strInfo + sz;
	
	const char* lpBuffer;
	UINT uiBytes;
	if( !VerQueryValueA( m_sBuf.buf(), (LPTSTR)(LPCTSTR)strBlock.c_str(), (void**)&lpBuffer, &uiBytes ) )
		return {};

	return {lpBuffer,uiBytes-1}; // -1 because of ending zero
}