%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="make_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x SLCOMMENT

SWC		[A-Za-z_]
WWC		[_A-Za-z0-9]
OC		[0-7]
DC		[0-9]
HC		[0-9A-Fa-f]
EOL		(\r?\n)
SPACE	[ \t]
WORD	{SWC}{WWC}*

%{

bool isMakeKeyword( const char* text ) 
{
	static CKeywordsIC kw("TXT_MAKE_KW");
	return kw.is(text); 
}

#define STYLE_TEXT			0
#define STYLE_KW			1
#define STYLE_COMMENT		3
#define STYLE_VAR			6
#define STYLE_SYMBOL		8

%}

%%
 /*************************************************************************************/

#						BEGIN(SLCOMMENT);	SetStyle();

$\([_a-zA-Z0-9]+\) |
$\(call\ my-dir\)		SetStyle(STYLE_VAR);		

<SLCOMMENT>{EOL}		BEGIN(INITIAL);	SetStyle();

<INITIAL>{
	ifneq				levelIncrease(); SetStyle(STYLE_KW);
	ifeq				levelIncrease(); SetStyle(STYLE_KW);
	else				levelDecrease(); levelIncrease(); SetStyle(STYLE_KW); 
	endif				levelDecrease(); SetStyle(STYLE_KW);

	^[a-z]+				SetStyle( isMakeKeyword(yytext) ? STYLE_KW : STYLE_TEXT );
	
	/* operator */
	[%\*\?<>|.\/\\:;=\+\*-]		SetStyle(STYLE_SYMBOL);
}

<*>{
	{WORD} |
	{SPACE}+ |
	{EOL} |
	.					SetStyle();
}

%%
/*************************************************************************************/

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case INITIAL:	return STYLE_TEXT;
		case SLCOMMENT:	return STYLE_COMMENT;
	}
	return 0;
}
