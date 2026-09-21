%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="cpp_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x COMMENT SLCOMMENT STRING CHAR PREPROCESSOR UID

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

bool isCppKeyword( const char* text, int lang ) 
{	
	switch( lang ) {
		case 1: { static CKeywords kw("TXT_CPP_KW"); return kw.is(text); }
		case 2: { static CKeywords kw("TXT_C_KW"); return kw.is(text); }
		case 3: { static CKeywords kw("TXT_CS_KW"); return kw.is(text); }
		case 4: { static CKeywords kw("TXT_IDL_KW"); return kw.is(text); }
		case 5: { static CKeywords kw("TXT_RC_KW"); return kw.is(text); }
		case 6: { static CKeywords kw("TXT_JAVA_KW"); return kw.is(text); }
		case 7: { static CKeywords kw("TXT_PROTO_KW"); return kw.is(text); }
	}
	return false;
}

#define STYLE_TEXT			0
#define STYLE_KW			1
#define STYLE_COMMENT		2
#define STYLE_SYMBOL		3
#define STYLE_STRING		4
#define STYLE_CHAR			5
#define STYLE_NUMBER		6
#define STYLE_PREP			7
#define STYLE_UNQ			8
#define STYLE_UID			9
#define STYLE_AT			10

%}

%%
 /*************************************************************************************/

<INITIAL>{	
	\/\/					BEGIN(SLCOMMENT); SetStyle();
	\/\*					BEGIN(COMMENT);   SetStyle(); levelIncrease();
	\"						BEGIN(STRING);	  SetStyle();
	'						BEGIN(CHAR);	  SetStyle();

	^{SPACE}#{SPACE}[a-z]+	{ //preprocessor
								char* pText;
								for( pText=yytext; *pText!='#'; pText++ );
								SetStyle( MatchStart(), MatchStart()+(pText-yytext), STYLE_TEXT );
								SetStyle( MatchStart()+(pText-yytext), MatchEnd(), STYLE_PREP );							
								BEGIN(PREPROCESSOR);
								pText++;
								bool bDec, bInc;
								checkPreprocessor( pText, m_langId, bDec, bInc );
								if( bDec ) levelDecrease();
								if( bInc ) levelIncrease();
							}

	uuid{SPACE}\(			{
			SetStyle(MatchStart(), MatchStart()+4, STYLE_KW );
			SetStyle(MatchEnd()-1, MatchEnd(), STYLE_SYMBOL );
			BEGIN(UID); 
	}

	/* VC++ message maps */
	^{SPACE}BEGIN_[A-Z_]+ 	SetStyle(STYLE_TEXT); levelIncrease(); 
	^{SPACE}END_[A-Z_]+		SetStyle(STYLE_TEXT); levelDecrease();

	/* RC */
	^{SPACE}BEGIN/{EOL} 	SetStyle(STYLE_KW); levelIncrease(); 
	^{SPACE}END/{EOL}		SetStyle(STYLE_KW); levelDecrease();

	@[a-zA-Z]+				SetStyle(STYLE_AT); 
	
	[_a-zA-Z][_a-zA-Z0-9]*		if( isCppKeyword(yytext,m_langId) ) SetStyle(STYLE_KW); else SetStyle();

	 /* numbers */
	[1-9]{DC}* |
	0{OC}* |
	0[Xx]{HC}+[Uu]? |
	{DC}+(\.{DC}+)?([Ee][+-]?{DC}+)?	SetStyle(STYLE_NUMBER);

	/* folding */
	[{]			SetStyle(STYLE_SYMBOL);	levelIncrease();
	[}]			SetStyle(STYLE_SYMBOL);	levelDecrease();
	
	/* folding */
	[\(]			SetStyle(STYLE_SYMBOL);	levelIncrease();
	[\)]			SetStyle(STYLE_SYMBOL);	levelDecrease();

	/* folding for IDL only ! */
	[\[]			SetStyle(STYLE_SYMBOL); levelIncrease();
	[\]]			SetStyle(STYLE_SYMBOL); levelDecrease();
	
	\/\/\{\{		BEGIN(SLCOMMENT); SetStyle(); levelIncrease();

	\/\/\}\}		BEGIN(SLCOMMENT); SetStyle(); levelDecrease();

	{SYMB}			SetStyle(STYLE_SYMBOL);
	
	 /* invalid chars - all second part of the ASCII table */
	[\x80-\xFF]+	SetStyle(IStyler::styleInvalidText);
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

<PREPROCESSOR>{
	/*folding*/
	start_map_region	SetStyle(); levelIncrease();
	stop_map_region		SetStyle(); levelDecrease();

	/* comments */
	/* \/\/				BEGIN(SLCOMMENT); SetStyle(); */
	/* \/\*				BEGIN(COMMENT);   SetStyle(); levelIncrease(); */

	\\{EOL}				SetStyle();
	{EOL}				BEGIN(INITIAL);   SetStyle();
}

<UID>{
	[0-9a-fA-F -]*	SetStyle();
	{EOL}			SetStyle(STYLE_UNQ); BEGIN(INITIAL);
	\)				SetStyle(STYLE_SYMBOL); BEGIN(INITIAL);
	.				SetStyle(IStyler::styleInvalidText);
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
		case PREPROCESSOR:	return STYLE_PREP;
		case UID:			return STYLE_UID;
	}
	return 0;
}