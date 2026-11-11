%{
#include "LexBase.h"
#include "stylesXHTML.h"
%}

 /* Go html/template: HTML (+ CSS, JS) of LexerXHTML.L with the {{...}} actions.
    an action can start in any HTML / CSS / JS state: the state is kept in m_stateExt (saved in
    the line state) and "}}" goes back to it. */

%option  c++
%option  prefix="gotpl_"
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
HTMX_ATTR	(data-)?hx-[A-Za-z_0-9:.-]*
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
	// template {{...}} states
%x TPL TPL_COMMENT TPL_STRING TPL_RAWSTRING TPL_CHAR

%{

/* style ids must match styleGoTpl in lexerStyles.cpp, the HTML / CSS / JS ones are from stylesXHTML.h */
#define STYLE_TPL				80	// parent: the background
#define STYLE_TPL_TEXT			81
#define STYLE_TPL_DELIM			82	// {{ }} {{- -}}
#define STYLE_TPL_KW			83
#define STYLE_TPL_BUILTIN		84
#define STYLE_TPL_FUNC			85
#define STYLE_TPL_VAR			86	// $x
#define STYLE_TPL_FIELD			87	// .Name
#define STYLE_TPL_STRING		88
#define STYLE_TPL_RAWSTRING		89
#define STYLE_TPL_CHAR			90
#define STYLE_TPL_NUMBER		91
#define STYLE_TPL_COMMENT		92
#define STYLE_TPL_SYMBOL		93
#define STYLE_TPL_UNQ			94

bool isHTMLKeyword( const char* pText );	// LexerXHTML.L

bool isCssKeyword( const char* text );
bool isCssValue( const char* text );

extern bool g_isJSRegExp;
bool isJSKeyword( const char* szKW );

static CKeywords kwTplBlock( "if range with define block", true );
static CKeywords kwTpl( "else end template break continue nil true false", true );
static CKeywords kwTplBuiltin("and or not len index slice print printf println html js urlquery call eq ne lt le gt ge", true );
static CKeywords kwTplFunc( "tr dueCal frameKey hasDay weekDays isOK dayNames nextRun", true );

%}

%%

	int after_tag = INITIAL; // UNDEF, INITIAL or CSS or SCR_JS

 /* ========================================================================================
 // template action {{...}}: from every HTML / CSS / JS state
 // =======================================================================================*/

<INITIAL,UNDEF,COMMENT,INTAG,STRING,STRING2,INTAG_CSS_ATTR,INTAG_CSS_VALUE,CSS,CSS_SELECTOR,CSS_ATTR,CSS_VALUE,CSS_COMMENT,CSS_SL_COMMENT,CSS_COMMENTA,CSS_SL_COMMENTA,CSS_STRING,CSS_CHAR,JS,JS_SLCOMMENT,JS_COMMENT,JS_STRING,JS_CHAR,JS_REGEXP>{
	\{\{			m_stateExt = YY_START; BEGIN(TPL); SetStyle(STYLE_TPL_DELIM);
	 /* "{{-" trims the spaces before, "{{-3}}" is a number */
	\{\{-/[ \t\r\n]	m_stateExt = YY_START; BEGIN(TPL); SetStyle(STYLE_TPL_DELIM);
}

<TPL>{
	\}\}		|
	-\}\}		SetStyle(STYLE_TPL_DELIM); BEGIN(m_stateExt);

	\/\*		BEGIN(TPL_COMMENT); SetStyle(); levelIncrease();
	\"			BEGIN(TPL_STRING); SetStyle();
	`			BEGIN(TPL_RAWSTRING); SetStyle();
	'			BEGIN(TPL_CHAR); SetStyle();

	 /* folding: {{if}} ... {{else if}} ... {{else}} ... {{end}} */
	else{WS}+(if|with)	SetStyle(STYLE_TPL_KW); levelDecrease(); levelIncrease();
	{WORD}		{
		if( kwTplBlock.is(yytext) ) {
			SetStyle(STYLE_TPL_KW); levelIncrease();
		} else if( !strcmp(yytext,"end") ) {
			SetStyle(STYLE_TPL_KW); levelDecrease();
		} else if( !strcmp(yytext,"else") ) {
			SetStyle(STYLE_TPL_KW); levelDecrease(); levelIncrease();
		} else if( kwTpl.is(yytext) )
			SetStyle(STYLE_TPL_KW);
		else if( kwTplBuiltin.is(yytext) )
			SetStyle(STYLE_TPL_BUILTIN);
		else if( kwTplFunc.is(yytext) )
			SetStyle(STYLE_TPL_FUNC);
		else
			SetStyle();
	}

	\${WWC}*	SetStyle(STYLE_TPL_VAR);
	\.{WORD}	|
	\.			SetStyle(STYLE_TPL_FIELD);

	0[xX]{HC}+	|
	-?{DC}+(\.{DC}+)?([eE][+-]?{DC}+)?	SetStyle(STYLE_TPL_NUMBER);

	:=			|
	[()|=,]		SetStyle(STYLE_TPL_SYMBOL);
}

<TPL_COMMENT>{
	\*\/		SetStyle(); BEGIN(TPL); levelDecrease();
}
<TPL_STRING,TPL_CHAR>{
	\\.			SetStyle();
	{EOL}		SetStyle(STYLE_TPL_UNQ); BEGIN(TPL);
}
<TPL_STRING>{
	\"			SetStyle(); BEGIN(TPL);
}
<TPL_CHAR>{
	'			SetStyle(); BEGIN(TPL);
}
 /* a raw string may span lines */
<TPL_RAWSTRING>{
	`			SetStyle(); BEGIN(TPL);
}

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
	{HTMX_ATTR}	SetStyle( STYLE_TAG_HTMX );
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
	 /* the line of <style> / <script> is not EOL filled */
	>{EOL}		SetStyle( MatchStart(), MatchStart()+1, STYLE_SYMBOL ); SetStyle( MatchStart()+1, MatchEnd(), CSciStyler::styleEOL ); BEGIN(after_tag);
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
<CSS_SL_COMMENT>{EOL}		BEGIN(CSS); SetStyle( STYLE_CSS_COMMENT );

<CSS_ATTR>\/\*				BEGIN(CSS_COMMENTA);   SetStyle(); levelIncrease();
<CSS_COMMENTA>\*\/			SetStyle();	BEGIN(CSS_ATTR); levelDecrease();

<CSS_ATTR>\/\/				BEGIN(CSS_SL_COMMENTA); SetStyle();
<CSS_SL_COMMENTA>{EOL}		BEGIN(CSS_ATTR); SetStyle( STYLE_CSS_COMMENT );

<CSS_VALUE>\"				BEGIN(CSS_STRING);	  SetStyle();
<CSS_STRING>\"				SetStyle(); BEGIN(CSS_VALUE);

<CSS_VALUE>'				BEGIN(CSS_CHAR);	  SetStyle();
<CSS_CHAR>'					SetStyle(); BEGIN(CSS_VALUE);

<CSS_STRING,CSS_CHAR>{EOL}	SetStyle(STYLE_CSS_UNQ); BEGIN(CSS);

<CSS>{
	[A-Za-z][A-Za-z_0-9-]*		SetStyle(STYLE_CSS_SELECTOR);

	 /* invalid chars - all second part of the ASCII table */
	[\x80-\xFF]+		SetStyle(CSciStyler::styleInvalidText);
}

 /* ========================================================================================
 // JS
 // ========================================================================================*/

<JS>{
	\/\/\<!\[CDATA\[	BEGIN(JS_SLCOMMENT); SetStyle(); levelIncrease();
	\/\/\]\]\>			BEGIN(JS_SLCOMMENT); SetStyle(); levelDecrease();

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
	{EOL}	BEGIN(JS); SetStyle( STYLE_JS_COMMENT );
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
	\\.					SetStyle(CSciStyler::styleInvalidText);

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
	\\\\	SetStyle();  // "\\" escaped backslash
	\\\/	SetStyle();
	\/gi	SetStyle( MatchStart(), MatchEnd()-2, STYLE_JS_REGEXP ); SetStyle( MatchEnd()-2, MatchEnd(), STYLE_JS_REGEXP_BOLD ); BEGIN(JS);
	\/[gi]	SetStyle( MatchStart(), MatchEnd()-1, STYLE_JS_REGEXP ); SetStyle( MatchEnd()-1, MatchEnd(), STYLE_JS_REGEXP_BOLD ); BEGIN(JS);
	\/		SetStyle(); BEGIN(JS);
	{EOL}	SetStyle(STYLE_JS_UNQ); BEGIN(JS);
}

 /* ========================================================================================
 // default rule
 // ========================================================================================*/

 /* the CSS / JS blocks are EOL filled: the line end gets the style of the block */
<CSS,CSS_SELECTOR,CSS_ATTR,CSS_VALUE,CSS_COMMENT,CSS_COMMENTA,JS,JS_COMMENT>{EOL}	SetStyle();

<*>{
	{EOL}						SetStyle( CSciStyler::styleEOL );
	.		SetStyle();
}

%%

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
// template
		case TPL:			return STYLE_TPL_TEXT;
		case TPL_COMMENT:	return STYLE_TPL_COMMENT;
		case TPL_STRING:	return STYLE_TPL_STRING;
		case TPL_RAWSTRING:	return STYLE_TPL_RAWSTRING;
		case TPL_CHAR:		return STYLE_TPL_CHAR;
	}
	return 0;
}
