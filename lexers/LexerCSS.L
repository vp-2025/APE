%{
#include "LexBase.h"
#include "stylesXHTML.h"
%}

%option  c++
%option  prefix="css_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x SELECTOR ATTR VALUE COMMENT SL_COMMENT COMMENTA SL_COMMENTA STRING CHAR

SWC		[A-Za-z_]
WWC		[_A-Za-z0-9]
OC		[0-7]
DC		[0-9]
HC		[0-9A-Fa-f]
EOL		(\r?\n)
SPACE	[ \t]
WORD	{SWC}{WWC}*

%{

bool isCssKeyword( const char* text );
bool isCssValue( const char* text );

%}

%%
 /*************************************************************************************/

<INITIAL>\{				SetStyle(STYLE_CSS_SYMBOL); BEGIN(ATTR); levelIncrease();
<ATTR,VALUE>\}			SetStyle(STYLE_CSS_SYMBOL); BEGIN(INITIAL); levelDecrease();
<ATTR>:					SetStyle(STYLE_CSS_SYMBOL); BEGIN(VALUE);
<VALUE>;				SetStyle(STYLE_CSS_SYMBOL); BEGIN(ATTR);

<ATTR>{
	[A-Za-z][A-Za-z_0-9-]*	if(isCssKeyword(yytext)) SetStyle(STYLE_CSS_ATTR_KW); else SetStyle();
}
<VALUE>{
	[A-Za-z][A-Za-z_0-9-]*	if(isCssValue(yytext)) SetStyle(STYLE_CSS_VAL_KW); else SetStyle();
}
<VALUE>{
	{DC}+pt |
	{DC}+px |
	{DC}+cm |
	{DC}+mm |
	{DC}+em |
	{DC}+% |
	{DC}+ |
	#{HC}+				SetStyle(STYLE_CSS_NUMBER);
}

<INITIAL>\/\*			BEGIN(COMMENT);   SetStyle(); levelIncrease();
<COMMENT>\*\/			SetStyle();	BEGIN(INITIAL); levelDecrease();

<INITIAL>\/\/			BEGIN(SL_COMMENT); SetStyle();
<SL_COMMENT>{EOL}		SetStyle();	BEGIN(INITIAL);

<ATTR>\/\*				BEGIN(COMMENTA);   SetStyle(); levelIncrease();
<COMMENTA>\*\/			SetStyle();	BEGIN(ATTR); levelDecrease();

<ATTR>\/\/				BEGIN(SL_COMMENTA); SetStyle();
<SL_COMMENTA>{EOL}		SetStyle();	BEGIN(ATTR);

<VALUE>\"				BEGIN(STRING);	  SetStyle();
<STRING>\"				SetStyle(); BEGIN(VALUE);

<VALUE>'				BEGIN(CHAR);	  SetStyle();
<CHAR>'					SetStyle(); BEGIN(VALUE);

<STRING,CHAR>{EOL}		SetStyle(STYLE_CSS_UNQ); BEGIN(INITIAL);

<INITIAL>{
	[A-Za-z][A-Za-z_0-9-]*		SetStyle(STYLE_CSS_SELECTOR);
	
	 /* invalid chars - all second part of the ASCII table */
	[\x80-\xFF]+		SetStyle(IStyler::styleInvalidText);
}

<*>{
	{EOL} |
	.					SetStyle();
}

%%
/*************************************************************************************/

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case INITIAL:		return STYLE_CSS;
		case SELECTOR:		return STYLE_CSS_SELECTOR;
		case ATTR:			return STYLE_CSS_ATTR;
		case VALUE:			return STYLE_CSS_VAL;		
		case COMMENT:
		case SL_COMMENT:
		case SL_COMMENTA:
		case COMMENTA:		return STYLE_CSS_COMMENT;
		case STRING:		return STYLE_CSS_STRING;
		case CHAR:			return STYLE_CSS_CHAR;
	}
	return 0;
}
