%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="ini_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x SLCOMMENT

SWC		[A-Za-z_]
WWC		[_A-Za-z0-9]
DC		[0-9]
EOL		(\r?\n)
SPACE	[ \t]
WORD	{SWC}{WWC}*

%{
#define STYLE_TEXT			0
#define STYLE_COMMENT		1
#define STYLE_SECTION		2
#define STYLE_VALUE			3
#define STYLE_SYMBOL		4
%}

%%
 /*************************************************************************************/

<INITIAL>{
	;						BEGIN(SLCOMMENT); SetStyle();
	#						BEGIN(SLCOMMENT); SetStyle();
	
	\[.*\]	{
		SetStyle( MatchStart(), MatchStart()+1, STYLE_SYMBOL );
		SetStyle( MatchStart()+1, MatchEnd()-1, STYLE_SECTION );
		SetStyle( MatchEnd()-1, MatchEnd(), STYLE_SYMBOL );
		if( GetLevel() ) levelDecrease();
		levelIncrease();
	}

	=.*	{
		SetStyle( MatchStart(), MatchStart()+1, STYLE_SYMBOL );
		SetStyle( MatchStart()+1, MatchEnd(), STYLE_VALUE );
	}
}

<SLCOMMENT>{
	{EOL}		BEGIN(INITIAL); SetStyle(); 
}

<*>{
	{EOL} |
	.			SetStyle();
}

%%
/*************************************************************************************/

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case SLCOMMENT: return STYLE_COMMENT;
	}
	return STYLE_TEXT;
}
