#include "lexJS.h"
#include "shared/platform.h"
#include "lexers.h"
#include "shared/str.h"
#include <map>
#include <set>
using namespace std;

struct CParamDesc { 
	string sParam, sDesc; 
	CParamDesc() {}
	CParamDesc( const string& s1, const string& s2 ) { sParam = s1; sDesc = s2; }
};

struct CPrefixItem {
	vector<string> vPrefix;
	string sList;
	void clear() { vPrefix.clear(); sList.clear(); }
};

string sRootList;
map<string,string> mObjectList;
map<string,CParamDesc> mParamDesc;
vector<CPrefixItem> vPrefixList;

string set2str( const set<string>& st )
{
	return combine( st, ' ' );
}

set<string> str2set( const string& str )
{
	set<string> st;
	split( str, st, ' ' );
	return st;
}

string getPart( string& s, char ch )
{
	string s2;
	if( s.empty() ) return s2;
	size_t k = s.find(ch);
	if( k!=string::npos )
	{
		s2 = s.substr(0,k);
		s.erase( s.begin(), s.begin()+k+1 );
	} else {
		s2 = s;
		s.clear();
	}
	return s2;
}

bool useInRoot( string& s )
{
	if( s.empty() ) 
		return false;
	if( s[0]=='@' ) { // skip starting with @ (body,style,event)
		s.erase( s.begin(), s.begin()+1 );
		return false;
	}
	string s1 = s;
	getPart(s1,'?');
	if( stoi(s1)==5 ) // skip keywords
		return false;
	return true;
}

void jsInit() 
{
	static bool bInit = false;
	if( bInit ) return;
	bInit = true;

	string sData;
	if( !LoadTextFromFile(GetExePath()+"js_api.txt",sData) )
		sData = LoadTextFromResource("TXT_JS_API");

	sRootList.clear();
	mObjectList.clear();
	mParamDesc.clear();

	vector<string> vData;
	split( sData, vData, '\n' );
	string sIdent;
	set<string> stRoot;
	set<string> st;
	CPrefixItem prefix;
	bool bPrefix = false;
	for( unsigned i=0; i<vData.size(); i++ )
	{
		string& s = vData[i];
		rtrim(s);
		if( s.empty() || s[0]==';' || s[0]=='#' ) 
			continue; // skip comment or empty line
		if( s[0]!='\t' )
		{
			if( !sIdent.empty() )
			{
				if( bPrefix ) {
					prefix.sList = set2str(st);
					vPrefixList.push_back( prefix );
				} else
					mObjectList[ sIdent ] = set2str(st);
			}
			st.clear();
			sIdent = getPart(s,'\t');
			if( sIdent.find('=')!=string::npos ) {
				vector<string> v;
				split( sIdent, v, '=' );
				sIdent = v[0];
				st = str2set( mObjectList[ v[1] ] );
			}
			string sParam = getPart(s,'\t');
			string sDesc = getPart(s,'\t');
			bPrefix = !sIdent.empty() && sIdent[0]=='.';
			if( bPrefix )
			{
				prefix.clear();
				sIdent.erase( sIdent.begin(), sIdent.begin()+1 );
				split( sIdent, prefix.vPrefix, ';' );
			} else {
				if( useInRoot(sIdent) )
					stRoot.insert( sIdent );
				sIdent = getPart(sIdent,'?');
				if( !sParam.empty() || !sDesc.empty() ) 
					mParamDesc[ sIdent ] = CParamDesc(sParam,sDesc);
			}
		} else {
			trim(s);
			string sMember = getPart(s,'\t');
			string sParam = getPart(s,'\t');
			string sDesc = getPart(s,'\t');
			st.insert(sMember);
			sMember = getPart(sMember,'?');
			if( !sParam.empty() || !sDesc.empty() ) 
			{
				if( bPrefix )
					mParamDesc[ prefix.vPrefix[0]+'.'+sMember ] = CParamDesc(sParam,sDesc);
				else
					mParamDesc[ sIdent+'.'+sMember ] = CParamDesc(sParam,sDesc);
			}
		}
	}
	if( !sIdent.empty() )
	{
		if( bPrefix ) {
			prefix.sList = set2str(st);
			vPrefixList.push_back( prefix );
		} else
			mObjectList[ sIdent ] = set2str(st);
	}
	sRootList = set2str(stRoot);
}

///////////////////////////////////////
bool isPrefix( const string& sPrefix, const string& sObj )
{
	if( sPrefix.empty() || sObj.empty() ) return false;
	if( sPrefix==sObj ) return true;
	if( sObj.length()>sPrefix.length() && sObj.find(sPrefix)==0 )
	{
		char ch = sObj[sPrefix.length()];
		return (ch>='A' && ch<='Z') || (ch>='0' && ch<='9') || ch=='_';
	}
	return false;
}

bool findPrefix( const string& sObj, int& idx )
{
	for( size_t i=0; i<vPrefixList.size(); i++ )
		for( size_t j=0; j<vPrefixList[i].vPrefix.size(); j++ )
			if( isPrefix( vPrefixList[i].vPrefix[j], sObj ) )
			{
				idx = i;
				return true;
			}
	return false;
}
///////////////////////////////////////


void jsGetRootList( string& str )
{
	jsInit();
	str = sRootList;
}

void jsGetRootParams( const string& sMember, string& sPrm, string& sDesc )
{
	jsInit();
	CParamDesc& pd = mParamDesc[sMember];
	sPrm = pd.sParam;
	sDesc = pd.sDesc;
}

void jsGetObjectList( const string& sObject, string& str )
{
	jsInit();
	map<string,string>::iterator it = mObjectList.find(sObject);
	int idx;
	if( it!=mObjectList.end() )
		str = it->second;
	else
	if( findPrefix(sObject,idx) )
		str = vPrefixList[idx].sList;
	else
		str.clear();
}

void jsGetObjectParams( const string& sObject1, const string& sMember, string& sPrm, string& sDesc )
{
	jsInit();
	string sObject = sObject1;
	int idx;
	if( findPrefix(sObject,idx) )
		sObject = vPrefixList[idx].vPrefix[0];
	CParamDesc& pd = mParamDesc[sObject+'.'+sMember];
	sPrm = pd.sParam;
	sDesc = pd.sDesc;
}

bool g_isJSRegExp = false;
bool isJSKeyword(const char* szKW) 
{
	jsInit();
	g_isJSRegExp = !strcmp(szKW,"match") || !strcmp(szKW,"search") || !strcmp(szKW,"replace") || !strcmp(szKW,"split");
	return mObjectList.find(szKW)!=mObjectList.end();
}

#define JQEURY "$()"

void jsFix4JQuery( string& sObject, bool bDot )
{
	if( sObject == JQEURY ) return;
	int k;
	if( findPrefix(sObject,k) ) return;
	size_t len = sObject.length();
	if( !len ) return;
	if( bDot ) {
		if( len>2 && sObject.substr(len-2,2)=="()" )
			sObject.erase( sObject.end()-2, sObject.end() );
		if( mParamDesc.find( string(JQEURY)+"."+sObject )!=mParamDesc.end() )
			sObject = JQEURY;
	} else {
		if( len>=2 && sObject[0]=='$' )
			sObject = JQEURY;
	}
}

bool isJSSkipStyle(CSciWrapper& sci, int pos, bool bStringChar)
{
	int style = sci.GetStyleAt(pos);
	if( sci.Lexer()==LEX_XHTML) {
		if( style<40 || style>=50 )
			return true;
		if( bStringChar )
			if( style==44 || style==45 )
				return true;
	}
	if( bStringChar && sci.Lexer()==LEX_JS )
		if( style==4 || style==5 )
			return true;
	return false;
}

bool isJSCode( CSciWrapper& sci )
{
	if( sci.Lexer()==LEX_JS ) {
		int style = sci.GetStyleAt(sci.GetCurrentPos());
		return style!=2 &&	// comment
			style!=4 &&		// string
			style!=5;		// char
	}
	if( sci.Lexer()==LEX_XHTML) {
		int style = sci.GetStyleAt(sci.GetCurrentPos());
		return style>=40 && style<50 && 
				style!=42 &&	// comment
				style!=44 &&	// string
				style!=45;		// char
	}
	return false;
}