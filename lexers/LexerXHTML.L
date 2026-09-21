%{
#include "LexBase.h"
#include "stylesXHTML.h"
%}

%option  c++
%option  prefix="xhtml_"
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
STYLE		[sS][tT][yY][lL][eE]
	// without "', esc - []-\/

%x COMMENT INTAG STRING STRING2 UNDEF 
	// intag CSS states
%x INTAG_CSS_ATTR INTAG_CSS_VALUE
	// CSS states
%x CSS CSS_SELECTOR CSS_ATTR CSS_VALUE CSS_COMMENT CSS_SL_COMMENT CSS_COMMENTA CSS_SL_COMMENTA CSS_STRING CSS_CHAR
	// JS States
%x JS JS_SLCOMMENT JS_COMMENT JS_STRING JS_CHAR JS_REGEXP
	// PHP States
%x PHP PHP_KEYWORD PHP_SLCOMMENT PHP_COMMENT PHP_STRING PHP_CHAR

%{

bool isHTMLKeyword( const char* pText  ) 
{ 
	static CKeywordsIC htmlKW("&nbsp; &amp; &lt; &gt; &quote; &apos; &quot; &curren; &middot;",true);
	return htmlKW.is(pText); 
}

bool isCssKeyword( const char* text );
bool isCssValue( const char* text );

extern bool g_isJSRegExp;
bool isJSKeyword( const char* szKW );

bool isPHPKeyword( const char* szKW )
{
	static CKeywords php("if else elseif define defined function return echo switch case default break foreach for substr strtolower strtoupper strlen empty true false global importClass class_exists new exit abstract class extends public protected private static array is_array",true);
	return php.is(szKW); 
}

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

<INITIAL,UNDEF>\<\?php	{
	BEGIN(PHP); SetStyle();
}

<INITIAL,UNDEF>\<\??{SPACE}{TAG_NAME}	{
		char* pText = yytext;
		pText++;
		if( *pText=='!' ) pText++;
		BEGIN(INTAG); 
		SetStyle( MatchStart(), MatchStart()+(pText-yytext), STYLE_SYMBOL );
		SetStyle( MatchStart()+(pText-yytext), MatchEnd(), STYLE_TAG_NAME );
		levelIncrease();
		after_tag = INITIAL;
		if( !strcmpi(pText,"script") )
			after_tag = JS;
		else
		if( !strcmpi(pText,"style") )
			after_tag = CSS;
	}

<INITIAL,UNDEF,JS,CSS>\<\/{SPACE}{TAG_NAME} {
		char* pText = yytext;
		pText += 2;
		BEGIN(INTAG); 
		SetStyle( MatchStart(), MatchStart()+(pText-yytext), STYLE_SYMBOL );
		SetStyle( MatchStart()+(pText-yytext), MatchEnd(), STYLE_TAG_NAME );
		levelDecrease();
		after_tag = INITIAL;
	}

<INTAG>{STYLE}{SPACE}={SPACE}\"	{
									SetStyle(MatchStart(), MatchStart()+5, STYLE_TAG_ATTR); 
									SetStyle(MatchStart()+5, MatchEnd()-1, STYLE_SYMBOL); 
									SetStyle(MatchEnd()-1, MatchEnd(), STYLE_CSS_SYMBOL); 
									BEGIN(INTAG_CSS_ATTR);
								}
<INTAG_CSS_ATTR>:					SetStyle(STYLE_CSS_SYMBOL); BEGIN(INTAG_CSS_VALUE);
<INTAG_CSS_VALUE>;					SetStyle(STYLE_CSS_SYMBOL); BEGIN(INTAG_CSS_ATTR);
<INTAG_CSS_ATTR,INTAG_CSS_VALUE>\"	SetStyle(STYLE_CSS_SYMBOL); BEGIN(INTAG);

<INTAG_CSS_ATTR>{
	[A-Za-z][A-Za-z_0-9-]*	if(isCssKeyword(yytext)) SetStyle(STYLE_CSS_ATTR_KW); else SetStyle();
}
<INTAG_CSS_VALUE>{
	[A-Za-z][A-Za-z_0-9-]*	if(isCssValue(yytext)) SetStyle(STYLE_CSS_VAL_KW); else SetStyle();
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
	\?>			SetStyle(STYLE_SYMBOL); BEGIN(after_tag); levelDecrease();
	\/>			SetStyle(STYLE_SYMBOL); BEGIN(after_tag); levelDecrease();
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
	[A-Za-z][A-Za-z_0-9-]*	if(isCssKeyword(yytext)) SetStyle(STYLE_CSS_ATTR_KW); else SetStyle();
}
<CSS_VALUE>{
	[A-Za-z][A-Za-z_0-9-]*	if(isCssValue(yytext)) SetStyle(STYLE_CSS_VAL_KW); else SetStyle();
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

<CSS>\/\/					BEGIN(CSS_SL_COMMENT); SetStyle();
<CSS_SL_COMMENT>{EOL}		BEGIN(CSS); SetStyle();

<CSS_ATTR>\/\*				BEGIN(CSS_COMMENTA);   SetStyle(); levelIncrease();
<CSS_COMMENTA>\*\/			SetStyle();	BEGIN(CSS_ATTR); levelDecrease();

<CSS_ATTR>\/\/				BEGIN(CSS_SL_COMMENTA); SetStyle();
<CSS_SL_COMMENTA>{EOL}		BEGIN(CSS_ATTR); SetStyle();

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
 // JS
 // ========================================================================================*/

<JS>{
	\/\/\<!\[CDATA\[	BEGIN(JS_SLCOMMENT); SetStyle(); levelIncrease(); 
	\/\/\]\]\>			BEGIN(JS_SLCOMMENT); SetStyle(); levelDecrease(); 
	
	\/\/\{\{			BEGIN(JS_SLCOMMENT); SetStyle(); levelIncrease();
	\/\/\}\}			BEGIN(JS_SLCOMMENT); SetStyle(); levelDecrease();
	
	\/\/		BEGIN(JS_SLCOMMENT); SetStyle();
	\/\*		BEGIN(JS_COMMENT); SetStyle(); levelIncrease();
	\/			if( g_isJSRegExp ) { BEGIN(JS_REGEXP); SetStyle(); } else SetStyle(STYLE_JS_SYMBOL);
	\"			BEGIN(JS_STRING); SetStyle();
	'			BEGIN(JS_CHAR); SetStyle();
	
	{WORD}		if( isJSKeyword(yytext) ) SetStyle( STYLE_JS_KW ); else SetStyle();
	[0-9]+		SetStyle(STYLE_JS_NUMBER);

	/* folding */
	[{}]		SetStyle(STYLE_JS_SYMBOL);	if( *yytext=='{' ) levelIncrease(); else levelDecrease();

	/* folding */
	[\(\)]		SetStyle(STYLE_JS_SYMBOL);	if( *yytext=='(' ) levelIncrease(); else { levelDecrease(); g_isJSRegExp=false; }

	/* folding */
	[\[\]]		SetStyle(STYLE_JS_SYMBOL);		if( *yytext=='[' ) levelIncrease(); else levelDecrease();

	{SYMB}		SetStyle(STYLE_JS_SYMBOL);
}

<JS_SLCOMMENT>{
	{EOL}	BEGIN(JS); SetStyle();
}
<JS_COMMENT>{
	\*\/	SetStyle(); BEGIN(JS); levelDecrease();
}
<JS_STRING,JS_CHAR>{
	/*escaped symbols*/
	\\[abfnrtv'"\/\\?] |
	\\u	|				// for JS unicode : \u04AF
	\\{OC}{OC}?{OC}? |
	\\[xX]{HC}{HC}		SetStyle();
	
	/*invalid escaped symbol*/
	\\.					SetStyle(IStyler::styleInvalidText);
	
	{EOL}	SetStyle(STYLE_JS_UNQ); BEGIN(JS);
}
<JS_STRING>{
	\\{EOL}	SetStyle(); BEGIN(JS);
	\"		SetStyle(); BEGIN(JS);
}
<JS_CHAR>{
	'		SetStyle(); BEGIN(JS);
}
<JS_REGEXP>{
	\\\\	SetStyle();  // "\\" escaped \
	\\\/	SetStyle();
	\/gi	SetStyle( MatchStart(), MatchEnd()-2, STYLE_JS_REGEXP ); SetStyle( MatchEnd()-2, MatchEnd(), STYLE_JS_REGEXP_BOLD ); BEGIN(JS);
	\/[gi]	SetStyle( MatchStart(), MatchEnd()-1, STYLE_JS_REGEXP ); SetStyle( MatchEnd()-1, MatchEnd(), STYLE_JS_REGEXP_BOLD ); BEGIN(JS);
	\/		SetStyle(); BEGIN(JS);
	{EOL}	SetStyle(STYLE_JS_UNQ); BEGIN(JS);
}

 /* ========================================================================================
 // PHP
 // ========================================================================================*/

 <PHP>{
	\?\>		BEGIN(INITIAL); SetStyle();
	\/\/		BEGIN(PHP_SLCOMMENT); SetStyle();
	\/\*		BEGIN(PHP_COMMENT); SetStyle(); levelIncrease();
	\"			BEGIN(PHP_STRING); SetStyle();
	'			BEGIN(PHP_CHAR); SetStyle();
	{WORD}		if( isPHPKeyword(yytext) ) SetStyle( STYLE_PHP_KW ); else SetStyle();
	[0-9]+		SetStyle(STYLE_PHP_NUMBER);
	[{}]		SetStyle(STYLE_PHP_SYMBOL); if( *yytext=='{' ) levelIncrease(); else levelDecrease();
 	{SYMB}		SetStyle(STYLE_PHP_SYMBOL);
}

<PHP_SLCOMMENT>{
	{EOL}		BEGIN(PHP); SetStyle();
}
<PHP_COMMENT>{
	\*\/		SetStyle(); BEGIN(PHP); levelDecrease();
}
<PHP_STRING,PHP_CHAR>{
	{EOL}		SetStyle(STYLE_PHP_UNQ); BEGIN(PHP);
}
<PHP_STRING>{
	\"			SetStyle(); BEGIN(PHP);
}
<PHP_CHAR>{
	'			SetStyle(); BEGIN(PHP);
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
		case INTAG_CSS_VALUE: return STYLE_CSS_VAL;
// CSS		
		case CSS:			return STYLE_CSS;
		case CSS_SELECTOR:	return STYLE_CSS_SELECTOR;
		case CSS_ATTR:		return STYLE_CSS_ATTR;
		case CSS_VALUE:		return STYLE_CSS_VAL;		
		case CSS_COMMENT:
		case CSS_SL_COMMENT:
		case CSS_SL_COMMENTA:
		case CSS_COMMENTA:	return STYLE_CSS_COMMENT;
		case CSS_STRING:	return STYLE_CSS_STRING;
		case CSS_CHAR:		return STYLE_CSS_CHAR;
// JS		
		case JS:			return STYLE_JS;
		case JS_SLCOMMENT:
		case JS_COMMENT:	return STYLE_JS_COMMENT;
		case JS_STRING:		return STYLE_JS_STRING;
		case JS_CHAR:		return STYLE_JS_CHAR;
		case JS_REGEXP:		return STYLE_JS_REGEXP;
// PHP		
		case PHP:			return STYLE_PHP;
		case PHP_SLCOMMENT:
		case PHP_COMMENT:	return STYLE_PHP_COMMENT;
		case PHP_STRING:	return STYLE_PHP_STRING;
		case PHP_CHAR:		return STYLE_PHP_CHAR;
	}
	return 0;
}