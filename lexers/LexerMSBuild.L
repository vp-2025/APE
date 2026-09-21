%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="msbuild_"
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
VAR			[$@%]\([a-zA-Z_.0-9]+\)

%x COMMENT
%x INTAG
%x STRING
%x STRING2

%{
#define STYLE_TEXT				0
//#define STYLE_KW				1
#define	STYLE_SYMBOL			2	// <>&=;\/
#define STYLE_COMMENT			3	// <!--...-->
#define STYLE_TAG_NAME			4
#define STYLE_TAG_ATTR			5
#define STYLE_TAG_STRING		6	// "..."
#define STYLE_TAG_NUMBER		7	// "..."
#define STYLE_TAG_TEXT			8
#define STYLE_VAR				9
#define STYLE_STRING_UNQ		12

%}

%%
 /*************************************************************************************/

 /* ========================================================================================
 // comment
 // =======================================================================================*/

<INITIAL>{
	\<!--		BEGIN(COMMENT); SetStyle(); levelIncrease();
}
<COMMENT>{
	-->			SetStyle(); BEGIN(INITIAL); levelDecrease();
}

<INITIAL>\<\??{SPACE}{TAG_NAME}	{
		char* pText = yytext;
		pText++;
		if( *pText=='?' ) pText++;
		BEGIN(INTAG); 
		SetStyle( MatchStart(), MatchStart()+(pText-yytext), STYLE_SYMBOL );
		SetStyle( MatchStart()+(pText-yytext), MatchEnd(), STYLE_TAG_NAME );
		levelIncrease();
}

<INITIAL>\<\/{SPACE}{TAG_NAME}	{
		char* pText = yytext;
		pText+=2;
		BEGIN(INTAG); 
		SetStyle( MatchStart(), MatchStart()+(pText-yytext), STYLE_SYMBOL );
		SetStyle( MatchStart()+(pText-yytext), MatchEnd(), STYLE_TAG_NAME );
		levelDecrease();
}

<INITIAL>{VAR}	SetStyle(STYLE_VAR);

<INTAG>{
	{VAR}		SetStyle(STYLE_VAR);
	
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
	\'			BEGIN(STRING2); SetStyle();		
	>			SetStyle(STYLE_SYMBOL); BEGIN(INITIAL); 
	\?>			SetStyle(STYLE_SYMBOL); BEGIN(INITIAL); levelDecrease();
	\/>			SetStyle(STYLE_SYMBOL); BEGIN(INITIAL); levelDecrease();
}

<STRING>{
	{VAR}		SetStyle(STYLE_VAR);
	{EOL}		SetStyle(STYLE_STRING_UNQ); BEGIN(INITIAL);
	\"			SetStyle(); BEGIN(INTAG);
}

<STRING2>{
	{VAR}		SetStyle(STYLE_VAR);
	{EOL}		SetStyle(STYLE_STRING_UNQ); BEGIN(INITIAL);
	\'			SetStyle(); BEGIN(INTAG);
}

<INITIAL>{
	[a-zA-Z0-9]+	SetStyle();
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
		case STRING2:	return STYLE_TAG_STRING;
		case INTAG:		return STYLE_TAG_TEXT;
	}
	return 0;
}