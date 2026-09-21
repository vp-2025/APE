%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="nsis_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x COMMENT SLCOMMENT STRING STRING2 STRING3 VAR PREPROCESSOR

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

#define STYLE_TEXT			0
#define STYLE_KW			1
#define STYLE_COMMENT		2
#define STYLE_SYMBOL		3
#define STYLE_STRING		4
#define STYLE_UNQ			5
#define STYLE_NUMBER		6
#define STYLE_VAR			7
#define STYLE_PREP			8
#define STYLE_LABEL			9

void checkNsisFoldLevel( const char* text, bool& bInc, bool& bDec ) {
	bInc = bDec = false;
	if( !strcmpi(text,"Section") || 
		!strcmpi(text,"SubSection") || 
		!strcmpi(text,"Function") || 
		!strcmpi(text,"SectionGroup")
		)
		bInc=true;
	if( !strcmpi(text,"SectionEnd") || 
		!strcmpi(text,"SubSectionEnd") || 		
		!strcmpi(text,"FunctionEnd") || 
		!strcmpi(text,"SectionGroupEnd")
		)
		bDec=true;
}

void checkNsisPrep( const char* text, bool& bDec, bool& bInc ) {
	bDec = bInc = false;
	char ch = text[0];

	switch( ch ) {
		case 'i': 
			if( !strcmp(text,"ifdef") || !strcmp(text,"ifndef") ) bInc = true;
			break;
		case 'e':
			if( !strcmp(text,"endif") ) bDec = true;
			if( !strcmp(text,"elif") || !strcmp(text,"else") ) {
				bDec=true; 
				bInc=true; 
			}
			break;
		case 'm':
			if( !strcmp(text,"macro") ) bInc=true;
			if( !strcmp(text,"macroend") ) bDec=true;
			break;
	}
}

bool isKeywordNSIS( const char* szKW ); // defined in lexNSIS

%}

%%
 /*************************************************************************************/

<INITIAL,PREPROCESSOR>{
	;					BEGIN(SLCOMMENT); SetStyle();
	#					BEGIN(SLCOMMENT); SetStyle();
	\/\*				BEGIN(COMMENT); SetStyle(); levelIncrease();
}

<INITIAL,STRING,STRING2,STRING3>{
	$\{{WORD}\}			SetStyle(STYLE_VAR);
	${WWC}+				SetStyle(STYLE_VAR);
	$\\[rn]				SetStyle(STYLE_VAR);
}

<INITIAL>{
	\"					BEGIN(STRING);	  SetStyle();
	\'					BEGIN(STRING2);	  SetStyle();
	\`					BEGIN(STRING3);	  SetStyle();

	^{SPACE}!{SPACE}[a-z]+	{ //preprocessor
								char* pText;
								for( pText=yytext; *pText!='!'; pText++ );
								SetStyle( MatchStart(), MatchStart()+(pText-yytext), STYLE_TEXT );
								SetStyle( MatchStart()+(pText-yytext), MatchEnd(), STYLE_PREP );							
								BEGIN(PREPROCESSOR);
								pText++;
								bool bDec, bInc;
								checkNsisPrep( pText, bDec, bInc );
								if( bDec ) levelDecrease();
								if( bInc ) levelIncrease();
							}

	^{SPACE}{WORD}{SPACE}:	SetStyle(STYLE_LABEL);

	{WORD}	{
				if( isKeywordNSIS(yytext) ) SetStyle(STYLE_KW); else SetStyle();
				bool bInc, bDec; 
				checkNsisFoldLevel(yytext,bInc,bDec);
				if( bDec ) levelDecrease();
				if( bInc ) levelIncrease();
			}	
	
	 /* numbers */
	[a-fA-F0-9]+		SetStyle(STYLE_NUMBER);

	{SYMB}				SetStyle(STYLE_SYMBOL);

	 /* invalid chars - all second part of the ASCII table */
	[\x80-\xFF]+		SetStyle(IStyler::styleInvalidText);
}

<COMMENT>{
	\*\/				SetStyle();	BEGIN(INITIAL); levelDecrease();
}

<SLCOMMENT>{
	{EOL}				BEGIN(INITIAL); SetStyle(); 
}

<STRING,STRING2,STRING3>{
	\\{EOL}			SetStyle();

	/*invalid end-of-line*/
	{EOL}				SetStyle(STYLE_UNQ); BEGIN(INITIAL);
}
<STRING>{
	\"					SetStyle(); BEGIN(INITIAL);
}
<STRING2>{
	\'					SetStyle(); BEGIN(INITIAL);
}
<STRING3>{
	\`					SetStyle(); BEGIN(INITIAL);
}

<PREPROCESSOR>{
	\\{EOL}			SetStyle();

	{EOL}				BEGIN(INITIAL);   SetStyle();
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
		case INITIAL:		return STYLE_TEXT;
		case COMMENT:		
		case SLCOMMENT:		return STYLE_COMMENT;
		case STRING:		
		case STRING2:		
		case STRING3:		return STYLE_STRING;
		case PREPROCESSOR:	return STYLE_PREP;
	}
	return 0;
}