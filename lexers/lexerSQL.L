%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="sql_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x COMMENT 
%x SLCOMMENT 
%x STRING 
%x CHAR

	// PL SQL
%x PLSQL
%x PLSQL_COMMENT 
%x PLSQL_SLCOMMENT 
%x PLSQL_STRING 
%x PLSQL_CHAR

SWC		[A-Za-z_]
WWC		[_A-Za-z0-9]
OC		[0-7]
DC		[0-9]
HC		[0-9A-Fa-f]
EOL		(\r?\n)
WS		[ \t]
SPACE	{WS}*
WORD	{SWC}{WWC}*
SYMB	[@<>(){}\[\]+\-*=&|~`\/\\,.:;!$?%^]
	// without - "'	esc []-\/   !

%{

bool isSQLKeyword( const char* text, bool& bInc, bool& bDec, bool bPLSQL ) 
{
	static CKeywordsIC kwSQL("TXT_SQL_KW");
	if( kwSQL.is(text) )
		return true;		
	static CKeywordsIC kwSQLFunc("TXT_SQLFUNC_KW");
	if( kwSQLFunc.is(text) )
		return true;
	if( !bPLSQL ) return false;
	static CKeywordsIC kwPLSQL("TXT_PLSQL_KW");
	return kwPLSQL.is(text);	
}

void proceedFolding( const char* text, bool& bInc, bool& bDec ) 
{
	if( strnicmp(text,"-- start section",16)==0 )
		bInc = true;
	if( strnicmp(text,"-- end section",14)==0 )
		bDec = true;
}

bool isBeginPLSQL( const char* text )
{
	return strcmpi(text,"begin")==0 || strcmpi(text,"declare")==0;
}

#define STYLE_TEXT			0
#define STYLE_KW			1
#define STYLE_COMMENT		2
#define STYLE_SYMBOL		3
#define STYLE_STRING		4
#define STYLE_CHAR			5
#define STYLE_NUMBER		6
#define STYLE_PARAM			7
#define STYLE_UNQ			8

#define STYLE_PLSQL				20
#define STYLE_PLSQL_KW			21
#define STYLE_PLSQL_COMMENT		22
#define STYLE_PLSQL_SYMBOL		23
#define STYLE_PLSQL_STRING		24
#define STYLE_PLSQL_CHAR		25
#define STYLE_PLSQL_NUMBER		26
#define STYLE_PLSQL_PARAM		27
#define STYLE_PLSQL_UNQ			28

%}

%%
 /*************************************************************************************/

<INITIAL>{
	\/\*				BEGIN(COMMENT); SetStyle(); levelIncrease();

	/* folding */
	--[a-zA-Z_ ]*		{
						BEGIN(SLCOMMENT); SetStyle();
						bool bInc=false, bDec=false;
						proceedFolding(yytext,bInc,bDec);
						if( bDec ) levelDecrease();
						if( bInc ) levelIncrease();
						}	

	\"					BEGIN(STRING); SetStyle();
	\'					BEGIN(CHAR); SetStyle();

	[dD][eE][cC][lL][aA][rR][eE]/{EOL}	{
							SetStyle(STYLE_PLSQL_KW);
							BEGIN(PLSQL); 
							levelIncrease();
						}
	[bB][eE][gG][iI][nN]/{EOL}	{
							SetStyle(STYLE_PLSQL_KW);
							BEGIN(PLSQL); 
							levelIncrease();
						}

	[_a-zA-Z]+			{
							bool bInc=false, bDec=false;
							if( isSQLKeyword(yytext,bInc,bDec,false) ) SetStyle(STYLE_KW); else SetStyle();
							if( bDec ) levelDecrease();
							if( bInc ) levelIncrease();
						}

	&[a-zA-Z][a-zA-Z0-9_]*\.		SetStyle(STYLE_PARAM);

	 /* numbers */
	[1-9]{DC}* |
	0{OC}* |
	${HC}+ |
	{DC}+(\.{DC}+)?([Ee][+-]?{DC}+)?	SetStyle(STYLE_NUMBER);

	{SYMB}			SetStyle(STYLE_SYMBOL);

	 /* invalid chars - all second part of the ASCII table */
	[\x80-\xFF]+	SetStyle(IStyler::styleInvalidText);
}

<COMMENT>{
	\*\/		SetStyle(); levelDecrease(); BEGIN(INITIAL);
}

<PLSQL_COMMENT>{
	\*\/		SetStyle(); levelDecrease(); BEGIN(PLSQL);
}

<SLCOMMENT>{
	{EOL}		BEGIN(INITIAL); SetStyle(); 
}

<PLSQL_SLCOMMENT>{
	{EOL}		BEGIN(PLSQL); SetStyle(); 
}

<STRING>{
	\\{EOL}		SetStyle();
	\"			SetStyle(); BEGIN(INITIAL);
}

<PLSQL_STRING>{
	\\{EOL}		SetStyle();
	\"			SetStyle(); BEGIN(PLSQL);
}

<CHAR>{
	\\{EOL}		SetStyle();
	\'			SetStyle(); BEGIN(INITIAL);
}

<PLSQL_CHAR>{
	\\{EOL}		SetStyle();
	\'			SetStyle(); BEGIN(PLSQL);
}

<STRING,CHAR,PLSQL_STRING,PLSQL_CHAR>{
	/*invalid end-of-line*/
	{EOL}		SetStyle(STYLE_UNQ); BEGIN(INITIAL);
}

<PLSQL>{
	\/\*		BEGIN(PLSQL_COMMENT); SetStyle(); levelIncrease();

	--			BEGIN(PLSQL_SLCOMMENT); SetStyle();

	\"			BEGIN(PLSQL_STRING); SetStyle();
	\'			BEGIN(PLSQL_CHAR); SetStyle();

	[eE][nN][dD];/{EOL}\/	levelDecrease(); SetStyle(STYLE_PLSQL_KW); BEGIN(INITIAL); 

	[_a-zA-Z]+	{
						bool bInc=false, bDec=false;
						if( isSQLKeyword(yytext,bInc,bDec,true) ) SetStyle(STYLE_PLSQL_KW); else SetStyle();
						if( bDec ) levelDecrease();
						if( bInc ) levelIncrease();
					}

	&[a-zA-Z][a-zA-Z0-9]*\.		SetStyle(STYLE_PLSQL_PARAM);

	 /* numbers */
	[1-9]{DC}* |
	0{OC}* |
	${HC}+ |
	{DC}+(\.{DC}+)?([Ee][+-]?{DC}+)?	SetStyle(STYLE_PLSQL_NUMBER);

	{SYMB}		SetStyle(STYLE_PLSQL_SYMBOL);
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
		case COMMENT:		
		case SLCOMMENT:     return STYLE_COMMENT;
		case STRING:		return STYLE_STRING;
		case CHAR:			return STYLE_CHAR;
		case PLSQL:				return STYLE_PLSQL;
		case PLSQL_COMMENT:		
		case PLSQL_SLCOMMENT:   return STYLE_PLSQL_COMMENT;
		case PLSQL_STRING:		return STYLE_PLSQL_STRING;
		case PLSQL_CHAR:		return STYLE_PLSQL_CHAR;
	}
	return 0;
}
