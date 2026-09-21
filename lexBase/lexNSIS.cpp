#include "lexNSIS.h"
#include "shared/platform.h"
#include "shared/str.h"
using namespace std;

CMapNSIS g_mapNSIS;
string g_sApiNSIS;

void initNSIS() 
{
	static bool bInit = false;
	if( bInit ) return;
	bInit = true;

	string sData;
	if( !LoadTextFromFile(GetExePath()+"nsis_api.txt",sData) )
		sData = LoadTextFromResource("TXT_NSIS_API");

	vector<string> vData;
	split( sData, vData, '\n' );
	CItemNSIS* pItem = NULL;
	for( unsigned i=0; i<vData.size(); i++ )
	{
		string& s = vData[i];
		rtrim(s);
		if( s[0]==';' || s[0]=='#' || s.empty() ) 
			continue; // skip comment or empty line
		if( s[0]!='\t' )
		{
			string sName, sParams;
			int k = s.find(' ');
			if( k==string::npos ) { 
				sName = s; 
			} else { 
				sName = s.substr(0,k); 
				sParams = s.substr(k+1); 
			}
			g_mapNSIS[sName] = CItemNSIS(sParams);
			pItem = &g_mapNSIS[sName];
		} else {
			trim(s);
			if( !pItem ) continue;
			if( !pItem->sDesc.empty() ) pItem->sDesc += '\n';
			pItem->sDesc += s;
		}
	}

	g_sApiNSIS.clear();
	for( CMapNSIS::iterator it=g_mapNSIS.begin(); it!=g_mapNSIS.end(); it++ )
	{
		if( !g_sApiNSIS.empty() ) g_sApiNSIS += ' ';
		g_sApiNSIS += it->first;
	}
}

bool isKeywordNSIS(const char* szKW) 
{
	initNSIS();
	return g_mapNSIS.find(szKW)!=g_mapNSIS.end();
}
