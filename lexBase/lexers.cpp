#include "stdafx.h"
#include "lexers.h"
#include "fileMask.h"

#include "lexBase.h"
#include "myLexers.h"

#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	cpp_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<cpp_FlexLexer>	CLexerCPP;

#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	js_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<js_FlexLexer>	CLexerJS;

#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	xhtml_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<xhtml_FlexLexer>	CLexerXHTML;

#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	xml_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<xml_FlexLexer>	CLexerXML;

#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	css_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<css_FlexLexer>	CLexerCSS;

#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	md_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<md_FlexLexer>	CLexerMD;

#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	pascal_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<pascal_FlexLexer>	CLexerPascal;

#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	conf_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<conf_FlexLexer>	CLexerConf;

#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	ini_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<ini_FlexLexer>	CLexerIni;
/*
#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	txt_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<txt_FlexLexer>	CLexerTxt;
*/
#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	sql_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<sql_FlexLexer>	CLexerSQL;

#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	batch_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<batch_FlexLexer>	CLexerBatch;

#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	nsis_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<nsis_FlexLexer>	CLexerNSIS;

#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	make_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<make_FlexLexer>	CLexerMAKE;

#undef	yyFlexLexerOnce
#undef   yyFlexLexer
#define  yyFlexLexer	msbuild_FlexLexer
#include "FlexLexer.h"
typedef  CSciLexer<msbuild_FlexLexer>	CLexerMSBuild;

CLexerInfo g_lexers[]={
	{LEX_NONE,"None"},
	{LEX_SEPARATOR},
	{LEX_CPP,"C++", {"*.cpp","*.cc","*.cxx",0}, {"*.hpp","*.h","*.hh","*.hxx",0} },
	{LEX_C,"C", {"*.c",0}, {"*.h",0} },
	{LEX_CS,"C#", {"*.cs",0} },
	{LEX_IDL,"IDL", {"*.idl",0} },
	{LEX_RC,"Resource", {"*.rc",0} },
	{LEX_JAVA,"Java", {"*.java",0} },
	{LEX_JS,"JavaScript", {"*.js","*.json",0} },
	{LEX_PROTO,"ProtoBuf", {"*.proto",0} },
	{LEX_SEPARATOR},
	{LEX_XHTML,"HTML", {"*.html","*.htm","*.php",0} },
	{LEX_XML,"XML", {"*.xml","*.xsl","*.xsd","*.svg","*.osm","*.xul","*.vcproj*","*.vcxproj*",0} }, // xsl=XML stylesheet lang xsd=XML schema definition
	{LEX_CSS,"CSS", {"*.css",0} },
	{LEX_MD,"Markdown", {"*.md","*.markdown","*.mdown","*.mkd","*.mdx",0} },
	{LEX_SEPARATOR},
	{LEX_PASCAL,"Pascal", {"*.pas","*.inc",0} },
	{LEX_SQL,"SQL", {"*.sql",0} },
	{LEX_INI,"Ini", {"*.ini","*.reg","*.inf","hgrc","*.properties","*.theme",0} },
	{LEX_CONF,"Conf", {"*.conf",".htaccess",0} },
	{LEX_NSIS,"NSIS", {"*.nsi",0}, {"*.nsh",0} },
	{LEX_BATCH,"Batch", {"*.bat","*.cmd",0} },
	{LEX_MAKE,"Makefile", {"*.mk","*.mak","makefile", "makefile.*",0} },
	{LEX_CMAKE,"CMake", {"cmakelists.txt",0} },
	{LEX_GRADLE,"Gradle",{ "*.gradle",0 } },
	{LEX_MSBUILD,"MSBuild", {"*.targets","*.props",0} },
	{LEX_SEPARATOR},
	{LEX_TXT,"Text", {"*.txt",".htaccess",".hgignore",".gitignore","readme","*.srt",0} },
	{LEX_TXT_NOQUOTE,"Text no quote", {NULL,0} },
	{0}
};

int DetectLexer( const string& sFile1 )
{
	string sFile = ExtractFileName(sFile1);
	for( CLexerInfo* pLex=g_lexers; pLex->iLexerID; pLex++ )
	{
		if( pLex->iLexerID==LEX_NONE || pLex->iLexerID==LEX_SEPARATOR )
			continue;
		for( const char** szMask=pLex->szSourceMasks; *szMask; szMask++ )
			if( isFileMask(*szMask,sFile) )
				return pLex->iLexerID;
		for( const char** szMask=pLex->szIncludeMasks; *szMask; szMask++ )
			if( isFileMask(*szMask,sFile) )
				return pLex->iLexerID;
	}
	return LEX_TXT; //LEX_NONE;
}

///////////////////////////////////////////////////////////////////////////////

class CSciStyler : public IStyler
{
	CSciWrapper& m_sci;
	int m_posStylingCur, m_posStylingTo;
public:
	explicit CSciStyler( CSciWrapper& sci ) : m_sci(sci) { m_posStylingCur = m_posStylingTo = 0; }

	int	 GetStylingPos() override { return m_posStylingCur; }
	void StartStyling(int posTo) override;
	int  GetText(char* buf, int max_size) override;
	int  GetStyle(int nPos) override { return m_sci.GetStyleAt( nPos ); }
	void SetStyle(int nStartPos, int nEndPos, int nStyle) override;
	int  GetLine(int nPos) override { return m_sci.LineFromPosition( nPos ); }
	void SetLevel(int nLine, int nLevel) override { m_sci.SetFoldLevel( nLine, nLevel ); }
	int  GetLevel(int nLine) override { return m_sci.GetFoldLevel( nLine ); }
	int  GetLineState(int nLine) override { return m_sci.GetLineState( nLine ); }
	void SetLineState(int nLine, int nState) override { m_sci.SetLineState( nLine, nState ); }
};

inline void CSciStyler::StartStyling(int posTo) {
	m_posStylingCur = m_sci.GetEndStyled();
	int nLine = m_sci.LineFromPosition(m_posStylingCur)-1;
	if( GetLevel(nLine) & SC_FOLDLEVELHEADERFLAG )
		nLine--;
	if( nLine<0 ) nLine=0;
	m_posStylingCur  = m_sci.PositionFromLine(nLine);
	m_posStylingTo = posTo;
	m_sci.StartStyling( m_posStylingCur, defaultStyleMask );
}

inline int CSciStyler::GetText(char* buf, int max_size) {
	if( m_posStylingCur >= m_posStylingTo )
		return 0; //EOF
	int cbAvail = m_posStylingTo - m_posStylingCur;
	if( cbAvail > max_size ) 
		cbAvail = max_size;
	
	int  posLast = m_posStylingCur + cbAvail;
	unsigned char chCur=0, chPrev=*(buf-1);
	for( int i = m_posStylingCur; i < posLast; i++, buf++, chPrev=chCur ) {
		 chCur = m_sci.GetCharAt(i);
		 if( chPrev == '\r' && chCur != '\n' )
			*(buf-1) = '\n';
		 *buf = chCur;
	}
	m_posStylingCur = posLast;
	return cbAvail;
}

inline void CSciStyler::SetStyle(int nStartPos, int nEndPos, int nStyle) {
	int nEndStyled = m_sci.GetEndStyled();
	if( nStartPos < nEndStyled )
		m_sci.StartStyling(nStartPos, defaultStyleMask);
	else
		if( nStartPos > nEndStyled )
			m_sci.SetStyling(nStartPos-nEndStyled, styleInvalidText);
	m_sci.SetStyling( nEndPos - nStartPos, nStyle & defaultStyleMask );
}

///////////////////////////////////////////////////////////////////////////////

void onStyleNeeded( CSciWrapper& sci, const string& sFile, int posTo ) 
{
	if( sci.Lexer()==LEX_NONE ) return;

	int line = sci.LineFromPosition(posTo);
	line += 5;
	posTo = max(posTo,sci.GetLineEndPosition(line));
	int pos = sci.GetEndStyled();

	CWaitCursor* pCur = nullptr;
	if( posTo-pos>1000*1000 ) pCur = new CWaitCursor();
	CSciStyler styler( sci );
	switch( sci.Lexer() )
	{
		case LEX_CPP:	{ CLexerCPP lexer( &styler, 1 ); lexer.Style( posTo ); } break;
		case LEX_C:		{ CLexerCPP lexer( &styler, 2 ); lexer.Style( posTo ); } break;
		case LEX_CS:	{ CLexerCPP lexer( &styler, 3 ); lexer.Style( posTo ); } break;
		case LEX_IDL:	{ CLexerCPP lexer( &styler, 4 ); lexer.Style( posTo ); } break;
		case LEX_RC:	{ CLexerCPP lexer( &styler, 5 ); lexer.Style( posTo ); } break;
		case LEX_JAVA:	{ CLexerCPP lexer( &styler, 6 ); lexer.Style( posTo ); } break;
		case LEX_PROTO:	{ CLexerCPP lexer( &styler, 7 ); lexer.Style( posTo ); } break;
		case LEX_JS:	{ CLexerJS lexer( &styler ); lexer.Style( posTo ); } break;

		case LEX_XHTML:	{ CLexerXHTML lexer( &styler, 0 ); lexer.Style( posTo ); } break;
//		case LEX_HTML:	{ CLexerHTML lexer( &styler, 0 ); lexer.Style( posTo ); } break;
		case LEX_XML:	{ CLexerXML lexer( &styler, 0 ); lexer.Style( posTo ); } break;
		case LEX_CSS:	{ CLexerCSS lexer( &styler, 0 ); lexer.Style( posTo ); } break;
		case LEX_MD:	{ CLexerMD lexer( &styler, 0 ); lexer.Style( posTo ); } break;

		case LEX_PASCAL:{ CLexerPascal lexer( &styler, 0 ); lexer.Style( posTo ); } break;
		case LEX_CONF:	{ 
			int lang=0;
			if( sFile.find("httpd.conf")!=string::npos || sFile.find("Apache")!=string::npos )
				lang=1; // apache
			else
			if( sFile.find("nginx")!=string::npos )
				lang=2; // nginx
			CLexerConf lexer( &styler, lang ); lexer.Style( posTo );
		} break;
		case LEX_INI:	{ CLexerIni lexer( &styler, 0 ); lexer.Style( posTo ); } break;

		case LEX_TXT: { CMyLexerText lexer(sci); lexer.StyleTo( posTo ); } break;
		case LEX_TXT_NOQUOTE:	{ CMyLexerText lexer(sci); lexer.setQuote(false); lexer.StyleTo( posTo ); } break;

		case LEX_BATCH: { CLexerBatch lexer( &styler, 0 ); lexer.Style( posTo ); } break;
		case LEX_SQL:	{ CLexerSQL lexer( &styler, 0 ); lexer.Style( posTo ); } break;
		case LEX_NSIS:	{ CLexerNSIS lexer( &styler, 0 ); lexer.Style( posTo ); } break;
		case LEX_MAKE:	{ CLexerMAKE lexer( &styler, 0 ); lexer.Style( posTo ); } break;
		case LEX_CMAKE: { CMyLexerCMake lexer( sci ); lexer.StyleTo( posTo ); } break;
		case LEX_GRADLE: { CMyLexerGradle lexer( sci ); lexer.StyleTo( posTo ); } break;

		case LEX_MSBUILD:	{ CLexerMSBuild lexer( &styler, 0 ); lexer.Style( posTo ); } break;
	} 
	if( pCur ) delete pCur;
}

void CLexerStyle::Init()
{
	isSize = iSize!=0;
	isFont = !sFont.empty();
	isFore = clrFore!=-1;
	isBack = clrBack!=-1;

	iMaskDef = iMask;
	iSizeDef = iSize;
	sFontDef = sFont;
	clrForeDef = clrFore;
	clrBackDef = clrBack;
}

void CLexerStyle::Load( CRegistry& reg, string key )
{
	key <<'_'<< id << '_';
	iMask = reg.ReadInt( key+"mask", iMask );
	if( IsSize() )
		iSize = reg.ReadInt( key+"size", iSize );
	if( IsFont() )
		sFont = reg.ReadStr( key+"font", sFont );
	if( IsClrFore() )
		clrFore = reg.ReadInt( key+"fore", clrFore );
	if( IsClrBack() )
		clrBack = reg.ReadInt( key+"back", clrBack );
}

bool CLexerStyle::Write( CRegistry& reg, string key ) const {
	if( !diff() )
		return false;
	key <<'_'<< id <<'_';
	if( diffMask() )
		reg.WriteInt( key+"mask", iMask );
	if( diffSize() )
		reg.WriteInt( key+"size", iSize );
	if( diffFont() )
		reg.WriteStr( key+"font", sFont );
	if( diffFore() )
		reg.WriteInt( key+"fore", clrFore );
	if( diffBack() )
		reg.WriteInt( key+"back", clrBack );
	return true;
}

void CIndicator::Init()
{
	colorDef = color;
	alphaDef = alpha;
	outlineAlphaDef = outlineAlpha;
	styleDef = style;
}

void CIndicator::Load( CRegistry& reg )
{
	string key;
	key << id << '_';
	color = reg.ReadInt( key+"color", color );
	alpha = reg.ReadInt( key+"alpha", alpha );
	outlineAlpha = reg.ReadInt( key+"outlineAlpha", outlineAlpha );
	style = reg.ReadInt( key+"style", style );
}

bool CIndicator::Write( CRegistry& reg ) const {
	if( !diff() )
		return false;
	string key;
	key << id << '_';
	if( diffColor() )
		reg.WriteInt( key+"color", color );
	if( diffAlpha() )
		reg.WriteInt( key+"alpha", alpha );
	if( diffOutlineAlpha() )
		reg.WriteInt( key+"outlineAlpha", outlineAlpha );
	if( diffStyle() )
		reg.WriteInt( key+"style", style );
	return true;
}

const char* STYLES = "styles";
const char* INDICATORS = "indicators";

void LoadLexerStyles()
{
	for( int i=0; g_styles[i].szName; i++ )
		for( int j=0; g_styles[i].styles[j].szName; j++ )
			g_styles[i].styles[j].Init();
	{
		CRegistry reg;
		if( reg.Open( g_hKey, string(g_szPath)+STYLES ) )
			for( int i=0; g_styles[i].szName; i++ ) {
				int k = reg.ReadInt( g_styles[i].szID, 0 );
				if( !k ) continue;
				for( int j=0; g_styles[i].styles[j].szName; j++ )
					g_styles[i].styles[j].Load( reg, g_styles[i].szID );
			}
	}

	for( int i=0; g_indicators[i].szName; i++ )
		g_indicators[i].Init();
	{
		CRegistry reg;
		if( reg.Open( g_hKey, string(g_szPath)+INDICATORS ) )
			for( int i=0; g_indicators[i].szName; i++ )
				g_indicators[i].Load( reg );
	}
}

void WriteLexerStyles()
{
	{
		CRegistry reg;
		CRegistry::Delete(g_hKey, string(g_szPath)+STYLES );
		if( reg.Create( g_hKey, string(g_szPath)+STYLES ) )
			for( int i=0; g_styles[i].szName; i++ ) {
				int k = 0;
				for( int j=0; g_styles[i].styles[j].szName; j++ )
					if( g_styles[i].styles[j].Write( reg, g_styles[i].szID ) )
						k++;
				if( k )
					reg.WriteInt( g_styles[i].szID, k );
			}
	}
	{
		CRegistry reg;
		CRegistry::Delete(g_hKey, string(g_szPath)+INDICATORS );
		if( reg.Create( g_hKey, string(g_szPath)+INDICATORS ) )
			for( int i=0; g_indicators[i].szName; i++ )
				g_indicators[i].Write( reg );
	}
}

///////////////////////////////////////////////////////////////////////////////

string doAlternate( const string& sFile, const char* szMask1[], const char* szMask2[] )
{
	bool b=false;
	for( int i=0; szMask1[i]; i++ )
		if( isFileMask(szMask1[i], sFile) )
		{
			b=true;
			break;
		}
	if( !b ) 
		return "";
	string sFileNameNoExt = ExtractFileNameWithoutExt(sFile);
	for( int i=0; szMask2[i]; i++ ) {
		string sFile2 = sFileNameNoExt + '.' + ExtractFileExt(szMask2[i]);
		if( IsFileExists(sFile2) )
			return sFile2;
	}
	return "";
}

string doAlternate( const string& sFile )
{
	for( CLexerInfo* pLex=g_lexers; pLex->iLexerID; pLex++ )
	{
		string sFile2 = doAlternate( sFile, pLex->szIncludeMasks, pLex->szSourceMasks );
		if( !sFile2.empty() )
			return sFile2;
		sFile2 = doAlternate( sFile, pLex->szSourceMasks, pLex->szIncludeMasks );
		if( !sFile2.empty() )
			return sFile2;
	}
	return "";
}