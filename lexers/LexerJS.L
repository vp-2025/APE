%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="js_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x COMMENT SLCOMMENT STRING CHAR REGEXP

SWC		[A-Za-z_]
WWC		[_A-Za-z0-9]
OC		[0-7]
DC		[0-9]
HC		[0-9A-Fa-f]
EOL		(\r?\n)
WS		[ \t]
SPACE	{WS}*
WORD	{SWC}{WWC}*
SYMB	[<>(){}\[\]+\-*=&|~`\/\\,.:;!$?%]
	// without - "'	esc []-\/   !

%{

extern bool g_isJSRegExp;
bool isJSKeyword( const char* szKW );

#define STYLE_TEXT			0
#define STYLE_KW			1
#define STYLE_COMMENT		2
#define STYLE_SYMBOL		3
#define STYLE_STRING		4
#define STYLE_CHAR			5
#define STYLE_REGEXP		11
#define STYLE_REGEXP_BOLD	12
#define STYLE_NUMBER		6
#define STYLE_UNQ			8

%}

%%
 /*************************************************************************************/

<INITIAL>{	
	\/\/					BEGIN(SLCOMMENT); SetStyle();
	\/\*					BEGIN(COMMENT);   SetStyle(); levelIncrease();
	\/						if( g_isJSRegExp ) { BEGIN(REGEXP); SetStyle(); } else SetStyle(STYLE_SYMBOL);
	\"						BEGIN(STRING);	  SetStyle();
	'						BEGIN(CHAR);	  SetStyle();

	[_a-zA-Z][_a-zA-Z0-9]*		if( isJSKeyword(yytext) ) SetStyle(STYLE_KW); else SetStyle();

	 /* numbers */
	[1-9]{DC}* |
	0{OC}* |
	0[Xx]{HC}+ |
	{DC}+(\.{DC}+)?([Ee][+-]?{DC}+)?	SetStyle(STYLE_NUMBER);

	/* folding */
	[{}]			SetStyle(STYLE_SYMBOL); if( *yytext=='{' ) levelIncrease(); else levelDecrease();

	/* folding */
	[\(\)]			SetStyle(STYLE_SYMBOL); if( *yytext=='(' ) levelIncrease(); else { levelDecrease(); g_isJSRegExp=false; }

	/* folding for IDL only ! */
	[\[\]]			SetStyle(STYLE_SYMBOL); if( *yytext=='[' ) levelIncrease(); else levelDecrease();
	
	{SYMB}			SetStyle(STYLE_SYMBOL);
	
	 /* invalid chars - all second part of the ASCII table */
	[\x80-\xFF]+		SetStyle(IStyler::styleInvalidText);
}

<SLCOMMENT>{
	{EOL}		BEGIN(INITIAL); SetStyle(); 
}
<COMMENT>{
	\*\/		SetStyle();	BEGIN(INITIAL); levelDecrease();
}

<STRING>{
	\\{EOL}		SetStyle(); BEGIN(INITIAL);
	\"			SetStyle(); BEGIN(INITIAL);
}

<CHAR>{
	'			SetStyle(); BEGIN(INITIAL);
}

<STRING,CHAR>{
	/*escaped symbols*/
	\\[abfnrtv'"\/\\?] |
	\\u	|				// for JS unicode : \u04AF
	\\{OC}{OC}?{OC}? |
	\\[xX]{HC}{HC}		SetStyle();

	/*invalid escaped symbol*/
	\\.					SetStyle(IStyler::styleInvalidText);

	/*invalid end-of-line*/
	{EOL}				SetStyle(STYLE_UNQ); BEGIN(INITIAL);
}

<REGEXP>{
	\\\\	SetStyle(); // "\\" escaped \
	\\\/	SetStyle();
	\/gi	SetStyle( MatchStart(), MatchEnd()-2, STYLE_REGEXP ); SetStyle( MatchEnd()-2, MatchEnd(), STYLE_REGEXP_BOLD ); BEGIN(INITIAL);
	\/[gi]	SetStyle( MatchStart(), MatchEnd()-1, STYLE_REGEXP ); SetStyle( MatchEnd()-1, MatchEnd(), STYLE_REGEXP_BOLD ); BEGIN(INITIAL);
	\/		SetStyle(); BEGIN(INITIAL);
	{EOL}	SetStyle(STYLE_UNQ); BEGIN(INITIAL);
}

<*>{
	{WORD} |
	{SPACE}+ |
	{EOL} |
	.				SetStyle();
}

%%
/*************************************************************************************/

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case INITIAL:		return STYLE_TEXT;
		case SLCOMMENT:
		case COMMENT:		return STYLE_COMMENT;
		case STRING:		return STYLE_STRING;
		case CHAR:			return STYLE_CHAR;
		case REGEXP:		return STYLE_REGEXP;
	}
	return 0;
}