%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="batch_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x SLCOMMENT ECHO STRING

SWC		[A-Za-z_]
WWC		[_A-Za-z0-9]
OC		[0-7]
DC		[0-9]
HC		[0-9A-Fa-f]
EOL		(\r?\n)
SPACE	[ \t]
WORD	{SWC}{WWC}*

%{

bool isBatchKeyword( const char* text ) 
{
	static CKeywordsIC kw("TXT_BATCH_KW");
	return kw.is(text); 
}

#define STYLE_TEXT			0
#define STYLE_KW			1
#define STYLE_EXTERNAL		2
#define STYLE_COMMENT		3
#define STYLE_ECHO			4
#define STYLE_STRING		5
#define STYLE_VAR			6
#define STYLE_LABEL			7
#define STYLE_SYMBOL		8
#define STYLE_STRING_UNQ	9

%}

%%
 /*************************************************************************************/

::						BEGIN(SLCOMMENT);	SetStyle();
\"						BEGIN(STRING);	SetStyle();
%[_a-zA-Z0-9\(\)]+%		SetStyle(STYLE_VAR);
%~dp[0-9]				SetStyle(STYLE_VAR);
%\*						SetStyle(STYLE_VAR);

<SLCOMMENT>{EOL}		BEGIN(INITIAL);	SetStyle();

<ECHO>{EOL}				BEGIN(INITIAL);	SetStyle();

<STRING>{
	%[_a-zA-Z0-9\(\)]+%		SetStyle(STYLE_VAR);
	%~dp[0-9]				SetStyle(STYLE_VAR);
	%\*						SetStyle(STYLE_VAR);
	{EOL}					SetStyle(STYLE_STRING_UNQ); BEGIN(INITIAL);
	\"						SetStyle(); BEGIN(INITIAL);
}

<INITIAL>{
	^{SPACE}*@?{SPACE}*[a-zA-Z0-9\/\\.]+	{
		char* pText;
		for( pText=yytext; strchr(" \t@",*pText)!=NULL ; pText++ );
		if( !strcmpi("rem",pText) ) { BEGIN(SLCOMMENT); SetStyle(); }
		else 
			if( !strcmpi("echo",pText) ) { BEGIN(ECHO); SetStyle(); }
			else 
				if( isBatchKeyword(pText) ) SetStyle(STYLE_KW); 
				else SetStyle(STYLE_EXTERNAL);
	}
	
	/* label */
	^{SPACE}*:{WORD}	SetStyle(STYLE_LABEL);

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
		case ECHO:		return STYLE_ECHO;
		case STRING:	return STYLE_STRING;
	}
	return 0;
}