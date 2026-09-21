#include "stdafx.h"
#include "lexBase.h"
#include "lexerStyles.h"

#define STYLE_INVALID IStyler::styleInvalidText
#define STYLE_WHITESPACE 40

#define clrWhite			RGB(0xFF,0xFF,0xFF)
#define clrBlack			RGB(0,0,0)
#define clrRed				RGB(0xFF,0,0)
#define clrGreen			RGB(0,0xFF,0)
#define clrBlue				RGB(0,0,0xFF)
#define clrDkGrey			RGB(0x60,0x60,0x60)
#define clrGrey				RGB(0x80,0x80,0x80)
#define clrLtGrey			RGB(0xA0,0xA0,0xA0)

#define clrComment			RGB(0x80,0x80,0x80)
#define clrKeyword			RGB(0x00,0x00,0xFF)
#define clrValue			RGB(0x00,0xB0,0x00)
#define clrVar				RGB(0x80,0x00,0xFF)
#define clrNumber			RGB(0x00,0x00,0x80)
#define clrString			RGB(0x80,0x80,0x00)
#define clrRegExp			RGB(0xFF,0x80,0x80)
#define clrChar				RGB(0xA0,0x60,0x00)
#define clrPrepocessor		RGB(0x00,0x00,0xFF)
#define clrSymbol			RGB(0x00,0x80,0x80)
#define clrOperator 		RGB(0x00,0xFF,0xFF)
#define clrLabel 			RGB(0x00,0x70,0x70)
#define clrUnquoted			RGB(0xFF,0x80,0x80)
#define clrInvalidFore	 	RGB(0xFF,0x00,0x00)
#define clrInvalidBack		RGB(0xFF,0xFF,0x00)

#define STL_BOLD			0x03
#define STL_ITALIC			0x0C
#define STL_UNDERLINE		0x30
#define STL_EOL				0xC0
#define STL_HOTSPOT			0x0300

#define STYLE_EX(id,szName,iSize,iFont)		{id,szName,0,-1,-1,iSize,iFont}

#define STYLE(id,szName)					{id,szName,0,-1,-1}
#define STYLE2(id,szName,clrFore)			{id,szName,0,clrFore,-1}
#define STYLE3(id,szName,clrFore,clrBack)	{id,szName,0,clrFore,clrBack}

#define STYLE2_BOLD(id,szName,clrFore)				{id,szName,STL_BOLD,clrFore,-1}
#define STYLE2_EX(id,szName,clrFore,iSize,iFont)	{id,szName,0,clrFore,-1,iSize,iFont}
#define STYLE3_EX(id,szName,clrFore,clrBack,iSize,iFont)	{id,szName,0,clrFore,clrBack,iSize,iFont}
#define STYLE3_BOLD(id,szName,clrFore,clrBack)		{id,szName,STL_BOLD,clrFore,clrBack}

#define STYLE_BOLD(id,szName,clrFore)		{id,szName,STL_BOLD,clrFore,-1}
#define STYLE_ITALIC(id,szName,clrFore)		{id,szName,STL_ITALIC,clrFore,-1}
#define STYLE_EOL(id,szName,clrFore,clrBack){id,szName,STL_EOL,clrFore,clrBack}
#define STYLE_LINK(id,szName,clrFore)		{id,szName,STL_HOTSPOT|STL_UNDERLINE,clrFore,-1}

CLexerStyle styleGlobal[] = {
	STYLE3_EX( STYLE_DEFAULT, "Default", 0x0, 0xFFFFFF, 11, "Consolas"),
	STYLE2_BOLD( STYLE_BRACELIGHT, "Brace Highlight", RGB(0xFF,0,0) ),
	STYLE3_BOLD( STYLE_BRACEBAD, "Brace Badlight", clrInvalidFore, clrInvalidBack ),
	STYLE2_EX( STYLE_LINENUMBER, "Line Number", RGB(0x40,0x40,0x40), 9, "Tahoma" ),
	STYLE2( STYLE_INDENTGUIDE, "Indent Guide", RGB(0xD0,0xD0,0xD0) ),
	STYLE3_EX( STYLE_CALLTIP, "Calltip Style", RGB(0,0,0), RGB(245,245,210), 8, "Tahoma" ),
//	STYLE( STYLE_CONTROLCHAR, "Control Chars" ), // only: font size bold italic
	STYLE2( STYLE_WHITESPACE, "White Space", clrLtGrey), // fore & back
	STYLE3( STYLE_INVALID, "Invalid Style", clrInvalidFore, clrInvalidBack ),
	{0}
};

CLexerStyle styleCPP[] = {
	STYLE( STYLE_DEFAULT, "Default"),
	STYLE( 0, "Text" ),
	STYLE_BOLD( 1, "Keyword",clrKeyword ),
	STYLE_ITALIC( 2, "Comment",clrComment ),
	STYLE2( 3, "Symbol",clrSymbol ),
	STYLE2( 4, "String",clrString ),
	STYLE2( 5, "Char",clrChar ),
	STYLE2( 6, "Number",clrNumber ),
	STYLE2( 7, "Preprocessor",clrPrepocessor ),
	STYLE_EOL( 8, "Unquoted Values",-1, clrUnquoted ),
	STYLE2( 9, "UID",0x808080 ),
	STYLE2( 10, "@", 0xA0A0A0 ),
	STYLE2( 11, "JS RegExp", clrRegExp ),
	STYLE_BOLD( 12, "JS RegExp Modifiers", clrRegExp ),
	{0}
};

#define clrTagAttr 0x800000
#define clrTagText 0x000080

CLexerStyle styleMSBuild[] = {
	STYLE( STYLE_DEFAULT, "Default"),
	STYLE( 0, "Text" ),
//	STYLE_BOLD( 1, "Keyword", clrKeyword ),
	STYLE2( 2, "Symbol", clrSymbol ),
	STYLE_ITALIC( 3, "Comment", clrComment ),
	STYLE2( 4, "Tag Name", clrKeyword ),
	STYLE2( 5, "Tag Attr", clrTagAttr ),
	STYLE2( 6, "Tag String", clrString ),
	STYLE2( 13, "Tag Char", clrChar ),
	STYLE2( 7, "Tag Number", clrNumber ),
	STYLE2( 8, "Tag Text", clrTagText ),
	STYLE2( 9, "Var", clrVar ),
	STYLE_EOL( 12, "Unquoted Values", -1, clrUnquoted ),
	{0}
};

#define STYLE_PARENT(id,name,clrBack) { id, name, STL_EOL, -1, clrBack, 0, "", 0 }

#define STYLE_CHILD(parentId,id,szName)						{id,szName,0,-1,-1,0,"",parentId}
#define STYLE_CHILD2(parentId,id,szName,clrFore)			{id,szName,0,clrFore,-1,0,"",parentId}
#define STYLE_CHILD3(parentId,id,szName,clrFore,clrBack)	{id,szName,0,clrFore,clrBack,0,"",parentId}

#define STYLE_CHILD_BOLD(parentId,id,szName,clrFore)		{id,szName,STL_BOLD,clrFore,-1,0,"",parentId}
#define STYLE_CHILD_ITALIC(parentId,id,szName,clrFore)		{id,szName,STL_ITALIC,clrFore,-1,0,"",parentId}
#define STYLE_CHILD_EOL(parentId,id,szName,clrFore,clrBack)	{id,szName,STL_EOL,clrFore,clrBack,0,"",parentId}

#define STYLE_JS_DEF		40
#define STYLE_CSS_DEF		50
#define STYLE_PHP_DEF		70

CLexerStyle styleHTML[] = {
	STYLE( STYLE_DEFAULT, "Default"),
	STYLE( 0, "Text" ),
	STYLE_BOLD( 1, "Keyword", clrKeyword ),
	STYLE2( 2, "Symbol", clrSymbol ),
	STYLE_ITALIC( 3, "Comment", clrComment ),
	STYLE2( 4, "Tag Name", clrKeyword ),
	STYLE2( 5, "Tag Attr", clrTagAttr ),
	STYLE2( 6, "Tag String", clrString ),
	STYLE2( 13, "Tag Char", clrChar ),
	STYLE2( 7, "Tag Number", clrNumber ),
	STYLE2( 8, "Tag Text", clrTagText ),
	STYLE3( 9, "Undef",-1, 0xA0A0A0 ),
	STYLE_EOL( 12, "Unquoted Values", -1, clrUnquoted ),

// CSS 50-61
	STYLE_PARENT( STYLE_CSS_DEF, "CSS", 0xFFF0F0 ),
	STYLE_CHILD_ITALIC( STYLE_CSS_DEF, 51, "Comment", clrComment ),
	STYLE_CHILD_BOLD( STYLE_CSS_DEF, 52, "Symbol", RGB(0,0x80,0) ),
	STYLE_CHILD2( STYLE_CSS_DEF, 53, "Number", clrNumber ),
	STYLE_CHILD2( STYLE_CSS_DEF, 54, "String", clrString ),
	STYLE_CHILD2( STYLE_CSS_DEF, 55, "Char", clrChar ),
	STYLE_CHILD_EOL( STYLE_CSS_DEF, 56, "Unquoted Values", -1, clrUnquoted ),
	STYLE_CHILD2( STYLE_CSS_DEF, 57, "Selector", RGB(0xE0,0,0) ),
	STYLE_CHILD2( STYLE_CSS_DEF, 58, "Attribute", RGB(0,0xA0,0) ),
	STYLE_CHILD_BOLD( STYLE_CSS_DEF, 59, "AttributeKW", RGB(0,0xA0,0) ),
	STYLE_CHILD2( STYLE_CSS_DEF, 60, "Value", RGB(0,0,0xE0) ),
	STYLE_CHILD_BOLD( STYLE_CSS_DEF, 61, "ValueKW", RGB(0,0,0xE0) ),

// JS 40-49
	STYLE_PARENT( STYLE_JS_DEF, "JS", 0xFFFBF0 ),
	STYLE_CHILD_BOLD( STYLE_JS_DEF, 41, "Keyword", clrKeyword ),
	STYLE_CHILD_ITALIC( STYLE_JS_DEF, 42, "Comment", clrComment ),
	STYLE_CHILD_BOLD( STYLE_JS_DEF, 43, "Symbol", RGB(0,0x80,0) ),
	STYLE_CHILD2( STYLE_JS_DEF, 44, "String", clrString ),
	STYLE_CHILD2( STYLE_JS_DEF, 45, "Char",	clrChar ),
	STYLE_CHILD2( STYLE_JS_DEF, 46, "JS RegExp", clrRegExp ),
	STYLE_CHILD_BOLD( STYLE_JS_DEF, 47, "JS RegExp Modifiers", clrRegExp ),
	STYLE_CHILD_EOL( STYLE_JS_DEF, 48, "Unquoted Values", -1, clrUnquoted ),
	STYLE_CHILD2( STYLE_JS_DEF, 49, "Number", clrNumber ),

// PHP 70-79
	STYLE_PARENT( STYLE_PHP_DEF, "PHP", 0xFFF0FB ),
	STYLE_CHILD_BOLD( STYLE_PHP_DEF, 71, "Keyword", clrKeyword ),
	STYLE_CHILD_ITALIC( STYLE_PHP_DEF, 72, "Comment", clrComment ),
	STYLE_CHILD_BOLD( STYLE_PHP_DEF, 73, "Symbol", RGB(0,0x80,0) ),
	STYLE_CHILD2( STYLE_PHP_DEF, 74, "String", clrString ),
	STYLE_CHILD2( STYLE_PHP_DEF, 75, "Char",	clrChar ),
	STYLE_CHILD_EOL( STYLE_PHP_DEF, 76, "Unquoted Values", -1, clrUnquoted ),
	STYLE_CHILD2( STYLE_PHP_DEF, 77, "Number", clrNumber ),
	{0}
};

CLexerStyle styleXML[] = {
	STYLE( STYLE_DEFAULT, "Default"),
	STYLE( 0, "Text" ),
//	STYLE_BOLD( 1, "Keyword", clrKeyword ),
	STYLE2( 2, "Symbol", clrSymbol ),
	STYLE_ITALIC( 3, "Comment", clrComment ),
	STYLE2( 4, "Tag Name", clrKeyword ),
	STYLE2( 5, "Tag Attr", clrTagAttr ),
	STYLE2( 6, "Tag String", clrString ),
	STYLE2( 13, "Tag Char", clrChar ),
	STYLE2( 7, "Tag Number", clrNumber ),
	STYLE2( 8, "Tag Text", clrTagText ),
	STYLE_EOL( 12, "Unquoted Values", -1, clrUnquoted ),
	{0}
};

CLexerStyle styleCSS[] = {
	STYLE( STYLE_DEFAULT, "Default"),
	STYLE( 50, "Text" ),
	STYLE_ITALIC( 51, "Comment", clrComment ),
	STYLE2_BOLD( 52, "Symbol", clrSymbol ),
	STYLE2( 53, "Number", RGB(0,0,0x80) ),
	STYLE2( 54, "String", clrString ),
	STYLE2( 55, "Char", clrChar ),
	STYLE_EOL( 56, "Unquoted Values", -1, clrUnquoted ),
	STYLE2( 57, "Selector", RGB(0xE0,0,0) ),
	STYLE2( 58, "Attribute", RGB(0,0xA0,0) ),
	STYLE_BOLD( 59, "AttributeKW", RGB(0,0xA0,0) ),
	STYLE2( 60, "Value", RGB(0,0,0xE0) ),
	STYLE_BOLD( 61, "ValueKW", RGB(0,0,0xE0) ),
	{0}
};

CLexerStyle stylePascal[] = {
	STYLE( STYLE_DEFAULT, "Default"),
	STYLE( 0, "Text" ),
	STYLE_BOLD( 1, "Keyword",clrKeyword ),
	STYLE_ITALIC( 2, "Comment",clrComment ),
	STYLE2( 3, "Symbol",clrSymbol ),
	STYLE2( 4, "String",clrString ),
	STYLE2( 5, "Char",clrChar ),
	STYLE2( 6, "Number",clrNumber ),
	STYLE_EOL( 8, "Unquoted Values",-1, clrUnquoted ),
	{0}
};

#define STYLE_PLSQL_DEF		20

CLexerStyle styleSQL[] = {
	STYLE( STYLE_DEFAULT, "Default"),
	STYLE( 0, "Text" ),
	STYLE_BOLD( 1, "Keyword", clrKeyword ),
	STYLE_ITALIC( 2, "Comment", clrComment ),
	STYLE2( 3, "Symbol", clrSymbol ),
	STYLE2( 4, "String", clrString ),
	STYLE2( 5, "Char", clrChar ),
	STYLE2( 6, "Number", clrNumber ),
	STYLE2( 7, "Parameter", clrKeyword ),
	STYLE_EOL( 8, "Unquoted Values",-1, clrUnquoted ),

// PLSQL 20-28
	STYLE_PARENT( STYLE_PLSQL_DEF, "PLSQL", 0xFFFBF0 ),
	//STYLE_CHILD( STYLE_PLSQL_DEF, 20, "Text" ),
	STYLE_CHILD_BOLD( STYLE_PLSQL_DEF, 21, "Keyword", clrKeyword ),
	STYLE_CHILD_ITALIC( STYLE_PLSQL_DEF, 22, "Comment", clrComment ),
	STYLE_CHILD_BOLD( STYLE_PLSQL_DEF, 23, "Symbol", 0 ),
	STYLE_CHILD2( STYLE_PLSQL_DEF, 24, "String", clrString ),
	STYLE_CHILD2( STYLE_PLSQL_DEF, 25, "Char",	clrChar ),
	STYLE_CHILD2( STYLE_PLSQL_DEF, 26, "Number", clrNumber ),
	STYLE_CHILD2( STYLE_PLSQL_DEF, 27, "Parameter", clrKeyword ),
	STYLE_CHILD_EOL( STYLE_PLSQL_DEF, 28, "Unquoted Values", -1, clrUnquoted ),
	{0}
};

CLexerStyle styleBatch[] = {
	STYLE( STYLE_DEFAULT, "Default"),
	STYLE( 0, "Text" ),
	STYLE_BOLD( 1, "System command", clrKeyword ),
	STYLE2( 2, "External command", clrNumber ),
	STYLE_ITALIC( 3, "Comment",	clrComment ),
	STYLE2( 4, "Echo", RGB(0x60,0x60,0x60) ),
	STYLE2( 5, "String", clrString ),
	STYLE2( 6, "Variable", RGB(0x80,0,0) ),
	STYLE2( 7, "Label", clrChar ),
	STYLE2( 8, "Symbol", clrSymbol ),
	STYLE_EOL( 9, "Unquoted Values", -1, clrUnquoted ),
	{0}
};

CLexerStyle styleConf[] = {
	STYLE( STYLE_DEFAULT, "Default"),
	STYLE( 0, "Text" ),
	STYLE_BOLD( 1, "Keyword1", clrKeyword ),
	STYLE2( 11, "Keyword2", clrKeyword ),
	STYLE_ITALIC( 2, "Comment", clrComment ),
	STYLE2( 3, "Symbol", clrSymbol ),
	STYLE2( 4, "String", clrString ),
	STYLE2( 6, "Number", clrNumber ),
	STYLE_EOL( 7, "Unquoted Values", -1, clrUnquoted ),
	STYLE2( 8, "Tag", clrKeyword ),
	STYLE2( 9, "Var", clrChar ),
	{0}
};

CLexerStyle styleIni[] = {
	STYLE( STYLE_DEFAULT, "Default"),
	STYLE( 0, "Text" ),
	STYLE_ITALIC( 1, "Comment", clrComment ),
	STYLE2( 2, "Section", RGB(0xA0,0,0) ),
	STYLE2( 3, "Value", RGB(0,0,0xA0) ),
	STYLE2( 4, "Symbol", clrSymbol ),
	{0}
};

CLexerStyle styleTxt[] = {
	STYLE( STYLE_DEFAULT, "Default"),
	STYLE( 0, "Text" ),
	STYLE2( 1, "Symbol", clrSymbol ),
	STYLE2( 2, "Number", clrNumber ),
	STYLE2( 3, "String", clrString ),
	// 4 Char
	STYLE_EOL( 5, "Unquoted Values", -1, clrUnquoted ),
	STYLE_LINK( 6, "Link", clrBlue ),
	{0}
};

CLexerStyle styleNSIS[] = {
	STYLE( STYLE_DEFAULT, "Default"),
	STYLE( 0, "Text"),
	STYLE_BOLD( 1, "Keyword", clrKeyword),
	STYLE_ITALIC( 2, "Comment", clrComment),
	STYLE2( 3, "Symbol", clrSymbol),
	STYLE2( 4, "String", clrString),
	STYLE_EOL( 5, "Unquoted Values", -1, clrUnquoted),
	STYLE2( 6, "Number", clrNumber),
	STYLE2( 7, "Var", clrChar),
	STYLE2( 8, "Preprocessor", clrPrepocessor),
	STYLE2( 9, "Label", clrLabel),
	{0}
};

CLexerStyle styleMAKE[] = {
	STYLE( STYLE_DEFAULT, "Default"),
	STYLE( 0, "Text"),
	STYLE_BOLD(1, "Keyword", clrKeyword),
	STYLE_ITALIC( 3, "Comment", clrComment),
	STYLE2( 6, "Var", clrChar),
	STYLE2( 8, "Symbol", clrSymbol),
	{0}
};

CLexerStyle styleGradle[] = {
	STYLE(STYLE_DEFAULT, "Default"),
	STYLE(0, "Text"),
	STYLE_BOLD(1, "Keyword", clrKeyword),
	STYLE_ITALIC(2, "Comment", clrComment),
	STYLE2(3, "Symbol", clrSymbol),
	STYLE2(4, "Number", clrNumber),
	STYLE2(5, "String", clrString),
	STYLE2(6, "Char", clrChar),
	STYLE_EOL(7, "Unquoted Values", -1, clrUnquoted),
	{ 0 }
};

CLexerStyles g_styles[] = {
	{"Global", "global", styleGlobal},
	{"C/C++", "cpp", styleCPP},
	{"HTML", "html", styleHTML},
	{"XML", "xml", styleXML},
	{"CSS", "css", styleCSS},
	{"Pascal", "pascal", stylePascal},
	{"SQL", "sql", styleSQL},
	{"Batch", "bat",styleBatch},
	{"Conf", "conf", styleConf},
	{"Ini", "ini", styleIni},
	{"Text","txt", styleTxt},
	{"NSIS", "nsis", styleNSIS},
	{"Make", "mak", styleMAKE},
	{ "Gradle", "gradle", styleGradle },
	{"MSBuild", "msbuild", styleMSBuild},
	{NULL}
};

/////////////////////////////////////////////////////////////////////

CLexerStyle* GetLexerStyles( CSciWrapper& sci )
{
	switch( sci.Lexer() )
	{
		case LEX_CPP:
		case LEX_C:
		case LEX_CS:
		case LEX_IDL:
		case LEX_RC:
		case LEX_JAVA:
		case LEX_PROTO: return styleCPP;

		case LEX_JS: return styleCPP;

		case LEX_XHTML: return styleHTML;
		case LEX_XML: return styleXML;
		case LEX_CSS: return styleCSS;

		case LEX_PASCAL: return stylePascal;
		case LEX_CONF: return styleConf;
		case LEX_INI: return styleIni;
		case LEX_TXT_NOQUOTE: 
		case LEX_TXT: return styleTxt;
		case LEX_SQL: return styleSQL;
		case LEX_BATCH: return styleBatch;
		case LEX_NSIS: return styleNSIS;
		case LEX_MAKE: return styleMAKE;
		case LEX_CMAKE: return styleMAKE;
		case LEX_GRADLE: return styleGradle;
		case LEX_MSBUILD: return styleMSBuild;
	};
	return NULL;
}

CLexerStyle* findStyle( CLexerStyle* pStyles, int id )
{
	for( ; pStyles && pStyles->szName; pStyles++ )
		if( pStyles->id==id )
			return pStyles;
	return NULL;
}

void applyStyle( CSciWrapper& sci, CLexerStyle* pStyle, int id=0  )
{
	if( !pStyle ) return;
	if( !id ) id=pStyle->id;
	CSciStyle& stl = sci.Style(id);
	if( pStyle->IsBold() )
		stl.Bold( pStyle->GetBold() );
	if( pStyle->IsUnderline() )
		stl.Underline( pStyle->GetUnderline() );
	if( pStyle->IsItalic() )
		stl.Italic( pStyle->GetItalic() );
	if( pStyle->IsEOL() )
		stl.EOL( pStyle->GetEOL() );
	if( pStyle->IsFont() )
		stl.Font( pStyle->GetFont() );
	if( pStyle->IsSize() )
		stl.Size( pStyle->GetSize() );
	if( pStyle->IsClrFore() )
		stl.Fore( pStyle->GetClrFore() );
	if( pStyle->IsClrBack() )
		stl.Back( pStyle->GetClrBack() );
	if( pStyle->isHotSpot )
		stl.HotSpot( pStyle->bHotSpot );
}

#include "xpm_img.h"

void SetupLexerStyles( CSciWrapper& sci )
{
	CLexerStyle* pStyles = GetLexerStyles(sci);

	sci.StyleResetDefault();
	applyStyle( sci, findStyle(styleGlobal,STYLE_DEFAULT) );
	applyStyle( sci, findStyle(pStyles,STYLE_DEFAULT) );
	sci.Call(SCI_STYLESETCHARACTERSET, STYLE_DEFAULT, SC_CHARSET_ANSI);
	sci.StylesClear();

	for( CLexerStyle* pStyle=styleGlobal; pStyle && pStyle->szName; pStyle++ ) {
		if( pStyle->id==STYLE_WHITESPACE ) {
			bool bFore = pStyle->isFore;
			bool bBack = pStyle->isBack;
			sci.Call( SCI_SETWHITESPACEFORE, bFore, pStyle->clrFore );
			sci.Call( SCI_SETWHITESPACEBACK, bBack, pStyle->clrBack );
		} else
		if( pStyle->id!=STYLE_DEFAULT )
			applyStyle( sci, pStyle );
	}

	for( CLexerStyle* pStyle=pStyles; pStyle && pStyle->szName; pStyle++ )
		if( pStyle->id!=STYLE_DEFAULT ) {
			if( pStyle->parentId ) {
				CLexerStyle* pParent = findStyle(pStyles, pStyle->parentId);
				applyStyle( sci, pParent, pStyle->id );
			}
			applyStyle( sci, pStyle );
		}

// cursor color: background inverted
	int clr = sci.StyleDefault().Back();
	clr = RGB( 255-GetRValue(clr), 255-GetGValue(clr), 255-GetBValue(clr) );
	sci.Call( SCI_SETCARETFORE, clr );

	sci.ClearRegisteredImages();
	if( sci.Lexer()==LEX_JS || sci.Lexer()==LEX_CSS || sci.Lexer()==LEX_XHTML )
	{
		sci.RegisterImage( 1, xpmData );
		sci.RegisterImage( 2, xpmMethod );
		sci.RegisterImage( 3, xpmEvent );
		sci.RegisterImage( 4, xpmClass );
		sci.RegisterImage( 5, xpmKW );
	}
}

// alpha: 0-10-..-90-100

CIndicator g_indicators[]={
	{INDIC+0, "Indicator1", RGB(0x80,0,0),		20, 40, INDIC_ROUNDBOX },
	{INDIC+1, "Indicator2", RGB(0,0x80,0),		20, 40, INDIC_ROUNDBOX },
	{INDIC+2, "Indicator3", RGB(0,0,0x80),		20, 40, INDIC_ROUNDBOX },
	{INDIC+3, "Indicator4", RGB(0x80,0x80,0),	20, 40, INDIC_ROUNDBOX },
	{INDIC+4, "Indicator5", RGB(0x80,0,0x80),	20, 40, INDIC_ROUNDBOX },
	{INDIC+5, "Indicator6", RGB(0,0x80,0x80),	20, 40, INDIC_ROUNDBOX },
	{INDIC+6, "Indicator7", RGB(0x40,0,0),		20, 40, INDIC_ROUNDBOX },
	{INDIC_FIND,		"Find",				RGB(0,0x80,0xFF), 20, 40, INDIC_PLAIN},
	{INDIC_REPLACE,		"Replace",			RGB(0xFF,0,0x80), 20, 40, INDIC_PLAIN},
	{INDIC_TOKEN,		"Token",			RGB(0xFF,0,0), 10, 30, INDIC_STRAIGHTBOX},
	{INDIC_TAGMATCH,	"Tag Match",		RGB(0,0xFF,0), 10, 30, INDIC_STRAIGHTBOX },
	{INDIC_TAGATTR,		"Tag Attributes",	RGB(0,0,0xFF), 10, 30, INDIC_STRAIGHTBOX },
	{ NULL }
};

void SetupIndicators( CSciWrapper& sci )
{
	for( int i=0; g_indicators[i].szName; i++ )
	{
		sci.Indicator( g_indicators[i].id )
			.Style( g_indicators[i].style )
			.Alpha( g_indicators[i].alpha*2.55 )
			.OutlineAlpha( g_indicators[i].outlineAlpha*2.55 )
			.Fore( g_indicators[i].color )
			.Under(true);
	}
}

