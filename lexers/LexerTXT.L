%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="txt_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x STRING

SWC		[A-Za-z_]
WWC		[_A-Za-z0-9]
DC		[0-9]
EOL		(\r?\n)
SPACE	[ \t]
WORD	{SWC}{WWC}*

%{
#define STYLE_TEXT			0
#define STYLE_SYMBOL		1
#define STYLE_NUMBER		2
#define STYLE_STRING		3
// #define STYLE_CHAR			4
#define STYLE_UNQ			5
%}

%%
 /*************************************************************************************/

<INITIAL>{
	/* esc []-/\ */
	[<>(){}\[\]+\-*=&|~`'\/\\,.:;%@?!#$^]		SetStyle(STYLE_SYMBOL);

	\"				if(m_bQuote) { BEGIN(STRING); SetStyle(); } else SetStyle(STYLE_SYMBOL);

	0[xX][a-fA-F0-9]+	|
	{DC}+			SetStyle(STYLE_NUMBER);

	{WORD}			SetStyle();
	
	[0-9]+{WORD}	SetStyle();
}

<STRING>{
	{EOL}		SetStyle(STYLE_UNQ); BEGIN(INITIAL);
	\"			SetStyle(); BEGIN(INITIAL);
}

<*>{
	{EOL} |
	.			SetStyle();
}

%%
/*************************************************************************************/

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case STRING: return STYLE_STRING; 
	}
	return STYLE_TEXT;
}