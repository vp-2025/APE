%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="pascal_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x COMMENT COMMENT2 SLCOMMENT STRING CHAR

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
IDENT	[_a-zA-Z][_a-zA-Z0-9]*

%{

bool isPascalKeyword( const char* text, bool& bInc, bool& bDec ) 
{
	if( !strcmpi(text,"class") || !strcmpi(text,"object") || !strcmpi(text,"dispinterface") || 
		!strcmpi(text,"record") || !strcmpi(text,"try") || !strcmpi(text,"case") || !strcmpi(text,"begin") ||
		!strcmpi(text,"interface") )
		bInc = true;
	if( !strcmpi(text,"end") )
		bDec = true;
	if( !strcmpi(text,"finally") || !strcmpi(text,"except") || !strcmpi(text,"implementation") ) {
		bDec = true;
		bInc = true;
	}
	static CKeywordsIC kwPascal("TXT_PASCAL_KW");
	return kwPascal.is(text);
}

#define STYLE_TEXT			0
#define STYLE_KW			1
#define STYLE_COMMENT		2
#define STYLE_SYMBOL		3
#define STYLE_STRING		4
#define STYLE_CHAR			5
#define STYLE_NUMBER		6
#define STYLE_UNQ			8

%}

%%
 /*************************************************************************************/

<INITIAL>{
	\/\/				BEGIN(SLCOMMENT); SetStyle();
	\{					BEGIN(COMMENT); SetStyle(); levelIncrease();
	\(\*				BEGIN(COMMENT2); SetStyle(); levelIncrease();
	'					BEGIN(STRING);	  SetStyle();

	={SPACE}interface{SPACE};	{
						char* pText;
						for( pText=yytext; *pText!='i'; pText++ );
						int len = (pText-yytext);
						SetStyle( MatchStart(), MatchStart()+1, STYLE_SYMBOL );
						SetStyle( MatchStart()+1, MatchStart()+len, STYLE_TEXT );
						SetStyle( MatchStart()+len, MatchStart()+len+9, STYLE_KW );
						SetStyle( MatchStart()+len+9, MatchEnd()-1, STYLE_TEXT );
						SetStyle( MatchEnd()-1, MatchEnd(), STYLE_SYMBOL );
						}

	={SPACE}interface	{
						char* pText;
						for( pText=yytext; *pText!='i'; pText++ );
						int len = (pText-yytext);
						SetStyle( MatchStart(), MatchStart()+1, STYLE_SYMBOL );
						SetStyle( MatchStart()+1, MatchStart()+len, STYLE_TEXT );
						SetStyle( MatchStart()+len, MatchEnd(), STYLE_KW );
						levelIncrease();
						}

	of{SPACE}object;	{ // sample : procedure ... of object;
						SetStyle( MatchStart(), MatchEnd()-1, STYLE_KW ); 
						SetStyle( MatchEnd()-1, MatchEnd(), STYLE_SYMBOL );
						}

	class{SPACE};		{
						SetStyle( MatchStart(), MatchStart()+5, STYLE_KW );
						SetStyle( MatchStart()+5, MatchEnd()-1, STYLE_TEXT );
						SetStyle( MatchEnd()-1, MatchEnd(), STYLE_SYMBOL );
						}

	class{SPACE}[oO][fF]{SPACE}{IDENT}; {
						SetStyle( MatchStart(), MatchStart()+8, STYLE_KW ); // class of
						SetStyle( MatchStart()+8, MatchEnd()-1, STYLE_TEXT ); // IDENT
						SetStyle( MatchEnd()-1, MatchEnd(), STYLE_SYMBOL );
						}

	class{SPACE}\({SPACE}{IDENT}{SPACE}\){SPACE}; {
						SetStyle( MatchStart(), MatchStart()+5, STYLE_KW );
						char* pText=yytext+5;
						for( ; *pText; pText++ ) {
							int p = MatchStart()+pText-yytext;
							SetStyle( p, p+1, (*pText=='(' || *pText==')') ? STYLE_SYMBOL : STYLE_TEXT );
						}
						SetStyle( MatchEnd()-1, MatchEnd(), STYLE_SYMBOL );
						}

	[eE][nN][dD].		{
						SetStyle( MatchStart(), MatchEnd()-1, STYLE_KW );
						SetStyle( MatchEnd()-1, MatchEnd(), STYLE_SYMBOL );
						levelDecrease();
						}

	[_a-zA-Z]+			{
						bool bInc=false, bDec=false;
						if( isPascalKeyword(yytext,bInc,bDec) ) SetStyle(STYLE_KW); else SetStyle();
						if( bDec ) levelDecrease();
						if( bInc ) levelIncrease();
						}

	 /* numbers */
	[1-9]{DC}* |
	0{OC}* |
	${HC}+ |
	{DC}+(\.{DC}+)?([Ee][+-]?{DC}+)?	SetStyle(STYLE_NUMBER);

	/* folding */
	[\(]			SetStyle(STYLE_SYMBOL);	levelIncrease();
	[\)]			SetStyle(STYLE_SYMBOL);	levelDecrease();
	[\[]			SetStyle(STYLE_SYMBOL);	levelIncrease();
	[\]]			SetStyle(STYLE_SYMBOL);	levelDecrease();

	{SYMB}			SetStyle(STYLE_SYMBOL);

	 /* invalid chars - all second part of the ASCII table */
	[\x80-\xFF]+	SetStyle(IStyler::styleInvalidText);
}

<SLCOMMENT>{
	{EOL}		BEGIN(INITIAL); SetStyle(); 
}
<COMMENT>{
	\}			SetStyle();	BEGIN(INITIAL); levelDecrease();
}
<COMMENT2>{
	\*\)		SetStyle();	BEGIN(INITIAL); levelDecrease();
}

<STRING>{
	\\{EOL}		SetStyle();
	\'			SetStyle(); BEGIN(INITIAL);
}

<STRING,CHAR>{
	/*invalid end-of-line*/
	{EOL}				SetStyle(STYLE_UNQ); BEGIN(INITIAL);
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
		case COMMENT2:
		case COMMENT:		return STYLE_COMMENT;
		case STRING:		return STYLE_STRING;
		case CHAR:			return STYLE_CHAR;
	}
	return 0;
}