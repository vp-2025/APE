#include "lexJS.h"
#include "lexers.h"

#include "stylesCSS.h"
#include "stylesXHTML.h"
#include "shared/str.h"
#include "shared/platform.h"

#include <map>
#include <set>
using namespace std;

string sList;

struct CCssKeyword {
	string desc, list;
	map<string,string> values;
};

map<string,CCssKeyword> mList;
set<string> stValues;

void cssInit() 
{
	static bool bInit = false;
	if( bInit ) return;
	bInit = true;

	string sData;
	if( !LoadTextFromFile(GetExePath()+"css_api.txt",sData) )
		sData = LoadTextFromResource("TXT_CSS_API");

	sList.clear();
	mList.clear();
	stValues.clear();

	CCssKeyword* pKW = NULL;
	vector<string> vData;
	split( sData, vData, '\n' );
	for( size_t i=0; i<vData.size(); i++ ) 
	{
		string& s = vData[i];
		rtrim(s);
		if( s.empty() || s[0]==';' || s[0]=='#' ) 
			continue; // skip comment or empty line
		if( s[0]!='\t' ) {
			vector<string> v;
			split( s, v, '\t' );
			if( v.empty() ) 
				continue;
			pKW = &mList[ v[0] ];
			if( v.size()>1 )
				pKW->desc = v[1];
		} else {
			if( !pKW ) continue;
			ltrim(s);
			vector<string> v;
			split( s, v, '\t' );
			if( v.empty() ) 
				continue;

			if( v.size()==1 && v[0].length()>3 ) { // reuse values
				s = v[0];
				if( s[0]=='$' && s[1]=='(' && s[s.size()-1]==')' ) {
					s = s.substr(2,s.size()-3);
					pKW->values = mList[s].values;
					continue;
				}
			}

			if( v.size()>1 )
				pKW->values[ v[0] ] = v[1];
			else
				pKW->values[ v[0] ];
			stValues.insert( v[0] );
		}
	}

	for( map<string,CCssKeyword>::iterator it=mList.begin(); it!=mList.end(); it++ )
	{
		if( !sList.empty() ) sList <<' ';
		sList << it->first << "?2";
		CCssKeyword& kw = it->second;
		for( map<string,string>::const_iterator it2=kw.values.begin(); it2!=kw.values.end(); it2++ )
		{
			if( !kw.list.empty() ) kw.list <<' ';
			kw.list << it2->first << "?1";
		}
	}
}

string cssGetKeywordList()
{
	cssInit();
	return sList;
}

string cssGetValueList( const string& sKeyword )
{
	cssInit();
	return mList[sKeyword].list;
}

string cssGetKeywordDesc( const string& sKeyword )
{
	cssInit();
	return mList[sKeyword].desc;
}

string cssGetValueDesc( const string& sKeyword, const string& sValue )
{
	cssInit();
	return mList[sKeyword].values[sValue];
}

bool isCssKeyword( const char* sz ) 
{
	cssInit();
	return mList.find(sz)!=mList.end();
}

bool isCssValue( const char* sz ) 
{
	cssInit();
	return stValues.find(sz)!=stValues.end();
}

bool isCssSymb(int sty) { return sty==STYLE_CSS_SYMBOL; }
bool isCssAttr(int sty) { return sty==STYLE_CSS_ATTR || sty==STYLE_CSS_ATTR_KW; }
bool isCssValue(int sty) { return sty==STYLE_CSS_NUMBER || sty==STYLE_CSS_VAL || sty==STYLE_CSS_VAL_KW; }

bool isCSSAttr( CSciWrapper& sci, int pos )
{
	if( pos<0 ) pos=sci.GetCurrentPos();
	int style = sci.GetStyleAt(pos);
	int ch = sci.GetCharAt(pos);
	int style1 = pos>0 ? sci.GetStyleAt(pos-1) : 0;
	int ch1 = pos>0 ? sci.GetCharAt(pos-1) : 0;

	if( sci.Lexer()==LEX_CSS || sci.Lexer()==LEX_XHTML ) {
		if( isCssSymb(style) && ch=='"' && isCssSymb(style1) && ch1=='"' )
			return true;
		if( isCssSymb(style) && ch=='}' && isCssSymb(style1) && ch1=='{' )
			return true;
		if( isCssSymb(style) && (ch=='}' || ch=='"') )
			return (isCssSymb(style1) && ch1==';') || isCssAttr(style1);
		return (isCssSymb(style) && ch==':') || isCssAttr(style);
	}
	return false;
}

bool isCSSValue( CSciWrapper& sci, int pos )
{
	if( pos<0 ) pos=sci.GetCurrentPos();
	int style = sci.GetStyleAt(pos);
	int ch = sci.GetCharAt(pos);
	int style1 = pos>0 ? sci.GetStyleAt(pos-1) : 0;
	int ch1 = pos>0 ? sci.GetCharAt(pos-1) : 0;

	if( sci.Lexer()==LEX_CSS || sci.Lexer()==LEX_XHTML ) {
		if( isCssSymb(style) && (ch=='}' || ch=='"') )
			return (isCssSymb(style1) && ch1==':') || isCssValue(style1);
		return (isCssSymb(style) && ch==';') || isCssValue(style);
	}
	return false;
}

string cssGetKeyword( CSciWrapper& sci )
{
	int p2 = sci.GetCurrentPos();
	for( ; p2 && sci.GetCharAt(p2)!=':'; p2-- );
	int p1 = p2-1;
	for( ; p1 && isCSSAttr(sci,p1); p1-- );
	string s = sci.GetTextRange(p1+1,p2);
	trim(s);
	return s;
}