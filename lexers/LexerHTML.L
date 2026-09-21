%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="html_"
%option  yyclass="CSciLexer<yyFlexLexer>"

SWC			[A-Za-z_]
WWC			[A-Za-z_0-9]
OC			[0-7]
DC			[0-9]
HC			[0-9A-Fa-f]
EOL			(\r?\n)
WS			[ \t]
SPACE		{WS}*
WORD		{SWC}{WWC}*
TAG_NAME	[A-Za-z_0-9][A-Za-z_0-9-]*
ATTR_NAME	{TAG_NAME}
SYMB		[()\[\]{}<>~@#$%^&|_+\-*=\/\\`.,:;!?]	
	// without "', esc - []-\/

%x COMMENT INTAG STRING STRING2 UNDEF 
	// intag CSS states
%x INTAG_CSS INTAG_CSS_ATTR INTAG_CSS_VALUE
	// CSS states
%x CSS CSS_SELECTOR CSS_ATTR CSS_VALUE CSS_COMMENT CSS_STRING CSS_CHAR
	// Script JS States
%x SCR_JS SCR_JS_SLCOMMENT SCR_JS_COMMENT SCR_JS_STRING SCR_JS_CHAR SCR_JS_EXPR

%{
#define STYLE_TEXT				0
#define STYLE_KW				1
#define	STYLE_SYMBOL			2	// <>&=;\/
#define STYLE_COMMENT			3	// <!--...-->
#define STYLE_TAG_NAME			4
#define STYLE_TAG_ATTR			5
#define STYLE_TAG_STRING		6	// "..."
#define STYLE_TAG_STRING2		13	// "..."
#define STYLE_TAG_NUMBER		7	// "..."
#define STYLE_TAG_TEXT			8
#define STYLE_UNDEF				9
#define STYLE_STRING_UNQ		12

// CSS
#define STYLE_CSS				50
#define STYLE_CSS_COMMENT		51
#define STYLE_CSS_SYMBOL		52
#define STYLE_CSS_NUMBER		53
#define STYLE_CSS_STRING		54
#define STYLE_CSS_CHAR			55
#define STYLE_CSS_UNQ			56
#define STYLE_CSS_SELECTOR		57
#define STYLE_CSS_ATTR			58
#define STYLE_CSS_KW			59
#define STYLE_CSS_VALUE			60
#define STYLE_CSS_VAL			61

// Script JS
#define STYLE_SCR_JS			40
#define STYLE_SCR_JS_KW			41
#define STYLE_SCR_JS_COMMENT	42
#define STYLE_SCR_JS_SYMBOL		43
#define STYLE_SCR_JS_STRING		44
#define STYLE_SCR_JS_CHAR		45
#define STYLE_SCR_JS_UNQ		46
#define STYLE_SCR_JS_NUMBER		47

static
bool isHTMLFold( const char* pText ) 
{ 
	static CKeywordsIC htmlFold("a b body div frameset form font h1 h2 h3 i p head html script style span td tr table title textarea tbody thead",true);
	return htmlFold.is(pText); 
}

static
bool isHTMLKeyword( const char* pText  ) 
{ 
	static CKeywordsIC htmlKW("&nbsp; &amp; &lt; &gt; &quote; &apos; &quot; &curren; &middot;",true);
	return htmlKW.is(pText); 
}

bool isCssKeyword( const char* text );
bool isCssValue( const char* text );

bool isJSKeyword( const char* szKW );

%}

%%
 /*************************************************************************************/

	int after_tag = INITIAL; // UNDEF, INITIAL or CSS or SCR_JS

 /* ========================================================================================
 // comment
 // =======================================================================================*/

<INITIAL,UNDEF>{
	\<!--		BEGIN(COMMENT); SetStyle(); levelIncrease();
}
<COMMENT>{
	-->			SetStyle(); BEGIN(INITIAL); levelDecrease();
}

<INITIAL,UNDEF>\<!{SPACE}{TAG_NAME}		{
		SetStyle( MatchStart(), MatchStart()+2, STYLE_SYMBOL );
		SetStyle( MatchStart()+2, MatchEnd(), STYLE_TAG_NAME );
		BEGIN(INTAG); 
}

<INITIAL,UNDEF>\<\??{SPACE}{TAG_NAME}	{
		char* pText = yytext;
		pText++;
		if( *pText=='!' ) pText++;
		BEGIN(INTAG); 
		SetStyle( MatchStart(), MatchStart()+(pText-yytext), STYLE_SYMBOL );
		SetStyle( MatchStart()+(pText-yytext), MatchEnd(), STYLE_TAG_NAME );
		bool bFold = isHTMLFold( pText );
		if( bFold ) levelIncrease();
		after_tag = INITIAL;
		if( !strcmpi(pText,"script") )
			after_tag = SCR_JS;
		else
		if( !strcmpi(pText,"style") )
			after_tag = CSS;
	}

<INITIAL,UNDEF,SCR_JS,CSS>\<\/{SPACE}{TAG_NAME} {
 // SCR_JS_SLCOMMENT,SCR_VB_SLCOMMENT - removed
		char* pText = yytext;
		pText += 2;
		BEGIN(INTAG); 
		SetStyle( MatchStart(), MatchStart()+(pText-yytext), STYLE_SYMBOL );
		SetStyle( MatchStart()+(pText-yytext), MatchEnd(), STYLE_TAG_NAME );
		bool bFold = isHTMLFold( pText );
		if( bFold ) levelDecrease();
		after_tag = INITIAL;
	}

<INTAG>[sS][tT][yY][lL][eE]{SPACE}=	SetStyle(MatchStart(), MatchEnd()-1, STYLE_TAG_ATTR); SetStyle(MatchEnd()-1, MatchEnd(), STYLE_SYMBOL); BEGIN(INTAG_CSS);
<INTAG_CSS>\"						SetStyle(STYLE_CSS_SYMBOL); BEGIN(INTAG_CSS_ATTR);
<INTAG_CSS_ATTR>:					SetStyle(STYLE_CSS_SYMBOL); BEGIN(INTAG_CSS_VALUE);
<INTAG_CSS_VALUE>;					SetStyle(STYLE_CSS_SYMBOL); BEGIN(INTAG_CSS_ATTR);
<INTAG_CSS_ATTR,INTAG_CSS_VALUE>\"	SetStyle(STYLE_CSS_SYMBOL); BEGIN(INTAG);

<INTAG_CSS_ATTR>{
	[A-Za-z][A-Za-z_0-9-]*	if(isCssKeyword(yytext)) SetStyle(STYLE_CSS_KW); else SetStyle();
}
<INTAG_CSS_VALUE>{
	[A-Za-z][A-Za-z_0-9-]*	if(isCssValue(yytext)) SetStyle(STYLE_CSS_VAL); else SetStyle();
}
<INTAG_CSS_VALUE>{
	{DC}+pt |
	{DC}+px |
	{DC}+cm |
	{DC}+mm |
	{DC}+em |
	{DC}+% |
	{DC}+ |
	#{HC}+					SetStyle(STYLE_CSS_NUMBER);
}

<INTAG>{
	{ATTR_NAME}	SetStyle( STYLE_TAG_ATTR );
	={SPACE}[0-9]+	|
	={SPACE}#[0-9a-fA-F]+	{
		SetStyle( MatchStart(), MatchStart()+1, STYLE_SYMBOL); 
		SetStyle( MatchStart()+1, MatchEnd(), STYLE_TAG_NUMBER);
	}
	={SPACE}[a-zA-Z0-9_][a-zA-Z0-9_%]*	{
		SetStyle( MatchStart(), MatchStart()+1, STYLE_SYMBOL); 
		SetStyle( MatchStart()+1, MatchEnd(), STYLE_TAG_TEXT);
	}
	=			SetStyle(STYLE_SYMBOL);
	\"			BEGIN(STRING); SetStyle();
	\'			BEGIN(STRING2);	SetStyle();
	>			SetStyle(STYLE_SYMBOL); BEGIN(after_tag); 
	\?>			SetStyle(STYLE_SYMBOL); BEGIN(after_tag); //levelDecrease();
	\/>			SetStyle(STYLE_SYMBOL); BEGIN(after_tag); //levelDecrease();
}

<STRING>{
	{EOL}		SetStyle(STYLE_STRING_UNQ); BEGIN(after_tag);
	\"			SetStyle(); BEGIN(INTAG);
}
<STRING2>{
	{EOL}		SetStyle(STYLE_STRING_UNQ); BEGIN(after_tag);
	\'			SetStyle(); BEGIN(INTAG);
}

<INITIAL>{
	&[a-zA-Z0-9]+; if( isHTMLKeyword(yytext) ) SetStyle(STYLE_KW); else SetStyle();
}

 /* ========================================================================================
 // CSS
 // ========================================================================================*/

<CSS>\{					SetStyle(STYLE_CSS_SYMBOL); BEGIN(CSS_ATTR); levelIncrease();
<CSS_ATTR,CSS_VALUE>\}	SetStyle(STYLE_CSS_SYMBOL); BEGIN(CSS); levelDecrease();
<CSS_ATTR>:				SetStyle(STYLE_CSS_SYMBOL); BEGIN(CSS_VALUE);
<CSS_VALUE>;			SetStyle(STYLE_CSS_SYMBOL); BEGIN(CSS_ATTR);

<CSS_ATTR>{
	[A-Za-z][A-Za-z_0-9-]*	if(isCssKeyword(yytext)) SetStyle(STYLE_CSS_KW); else SetStyle();
}
<CSS_VALUE>{
	[A-Za-z][A-Za-z_0-9-]*	if(isCssValue(yytext)) SetStyle(STYLE_CSS_VAL); else SetStyle();
}
<CSS_VALUE>{
	{DC}+pt |
	{DC}+px |
	{DC}+cm |
	{DC}+mm |
	{DC}+em |
	{DC}+% |
	{DC}+ |
	#{HC}+				SetStyle(STYLE_CSS_NUMBER);
}

<CSS>\/\*					BEGIN(CSS_COMMENT);   SetStyle(); levelIncrease();
<CSS_COMMENT>\*\/			SetStyle();	BEGIN(CSS); levelDecrease();

<CSS_VALUE>\"				BEGIN(CSS_STRING);	  SetStyle();
<CSS_STRING>\"				SetStyle(); BEGIN(CSS_VALUE);

<CSS_VALUE>'				BEGIN(CSS_CHAR);	  SetStyle();
<CSS_CHAR>'					SetStyle(); BEGIN(CSS_VALUE);

<CSS_STRING,CSS_CHAR>{EOL}	SetStyle(STYLE_CSS_UNQ); BEGIN(CSS);

<CSS>{
	[A-Za-z][A-Za-z_0-9-]*		SetStyle(STYLE_CSS_SELECTOR);
	
	 /* invalid chars - all second part of the ASCII table */
	[\x80-\xFF]+		SetStyle(IStyler::styleInvalidText);
}

 /* ========================================================================================
 // Script JS
	\/			BEGIN(SCR_JS_EXPR); SetStyle();
	\\'		SetStyle();
 // ========================================================================================*/

<SCR_JS>{
	\/\/\<!\[CDATA\[	BEGIN(SCR_JS_SLCOMMENT); SetStyle(); levelIncrease(); 
	\/\/\]\]\>			BEGIN(SCR_JS_SLCOMMENT); SetStyle(); levelDecrease(); 
	
	\/\/\{\{			BEGIN(SCR_JS_SLCOMMENT); SetStyle(); levelIncrease();
	\/\/\}\}			BEGIN(SCR_JS_SLCOMMENT); SetStyle(); levelDecrease();
	
	\/\/		BEGIN(SCR_JS_SLCOMMENT); SetStyle();
	\/\*		BEGIN(SCR_JS_COMMENT); SetStyle(); levelIncrease();
	\"			BEGIN(SCR_JS_STRING); SetStyle();
	'			BEGIN(SCR_JS_CHAR); SetStyle();
	
	{WORD}		if( isJSKeyword(yytext) ) SetStyle( STYLE_SCR_JS_KW ); else SetStyle();
	[0-9]+		SetStyle(STYLE_SCR_JS_NUMBER);
	[{}]		SetStyle(STYLE_SCR_JS_SYMBOL); if( *yytext=='{' ) levelIncrease(); else levelDecrease();
	{SYMB}		SetStyle(STYLE_SCR_JS_SYMBOL);
}

<SCR_JS_SLCOMMENT>{
	{EOL}	BEGIN(SCR_JS); SetStyle();
}
<SCR_JS_COMMENT>{
	\*\/	SetStyle(); BEGIN(SCR_JS); levelDecrease();
}
<SCR_JS_STRING,SCR_JS_CHAR>{
	/*escaped symbols*/
	\\[abfnrtv'"\/\\?] |
	\\u	|				// for JS unicode : \u04AF
	\\{OC}{OC}?{OC}? |
	\\[xX]{HC}{HC}		SetStyle();
	
	/*invalid escaped symbol*/
	\\.					SetStyle(IStyler::styleInvalidText);
	
	{EOL}	SetStyle(STYLE_SCR_JS_UNQ); BEGIN(SCR_JS);
}
<SCR_JS_STRING>{
	\\{EOL}	SetStyle(); BEGIN(SCR_JS);
	\"		SetStyle(); BEGIN(SCR_JS);
}
<SCR_JS_CHAR>{
	'		SetStyle(); BEGIN(SCR_JS);
}
<SCR_JS_EXPR>{
	\\[abfnrtv'"\/\\?]	SetStyle();
	\/		SetStyle(); BEGIN(SCR_JS);
	\/g		SetStyle(); BEGIN(SCR_JS);
}

 /* ========================================================================================
 // default rule
 // ========================================================================================*/

<*>{
	{EOL} |
	.		SetStyle();
}

%%
 /*************************************************************************************/

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case INITIAL:	return STYLE_TEXT;
		case COMMENT:	return STYLE_COMMENT;
		case STRING:	return STYLE_TAG_STRING;
		case STRING2:	return STYLE_TAG_STRING2;
		case INTAG:		return STYLE_TAG_TEXT;
		case UNDEF:		return STYLE_UNDEF;
		case INTAG_CSS_ATTR: return STYLE_CSS_ATTR;
		case INTAG_CSS_VALUE: return STYLE_CSS_VALUE;
// CSS		
		case CSS:			return STYLE_CSS;
		case CSS_SELECTOR:	return STYLE_CSS_SELECTOR;
		case CSS_ATTR:		return STYLE_CSS_ATTR;
		case CSS_VALUE:		return STYLE_CSS_VALUE;		
		case CSS_COMMENT:	return STYLE_CSS_COMMENT;
		case CSS_STRING:	return STYLE_CSS_STRING;
		case CSS_CHAR:		return STYLE_CSS_CHAR;
// Scr JS		
		case SCR_JS:			return STYLE_SCR_JS;
		case SCR_JS_SLCOMMENT:
		case SCR_JS_COMMENT:	return STYLE_SCR_JS_COMMENT;
		case SCR_JS_STRING:		return STYLE_SCR_JS_STRING;
		case SCR_JS_CHAR:		return STYLE_SCR_JS_CHAR;
		case SCR_JS_EXPR:		return STYLE_SCR_JS_STRING;
	}
	return 0;
}