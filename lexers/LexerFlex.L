%{
/******************************************************************************
* 		flex / lex input files (*.L)
*
* A .L file has three sections, separated by a line holding "%%":
*   1. definitions  - %{ ... %} C code, %option / %x / %s directives and
*                     "name    pattern" definitions
*   2. rules        - "pattern  action" pairs, <start condition> lists,
*                     SC scopes and C-style comments
*   3. user code    - C code copied verbatim into the scanner
*
* The pattern of a rule ends at the first whitespace, everything after it is
* the action.  An action that opens with "{" is highlighted as a C block: its
* braces are counted so that a nested "{ ... }" does not end it.
******************************************************************************/
#include "LexBase.h"
%}

%option  c++
%option  prefix="flex_"
%option  yyclass="CSciLexer<yyFlexLexer>"

SWC			[A-Za-z_]
WWC			[A-Za-z_0-9]
DC			[0-9]
EOL			(\r?\n)
WS			[ \t]
WORD		{SWC}{WWC}*
NAME		{SWC}{WWC}*
NUM			{DC}+
SC_LIST		("*"|"^"|{NAME}(","{NAME})*)

%x PROLOGD
%x PROLOGR
%x RULES
%x ACTION
%x CODE
%x COMMENT

%{
/* style ids - keep in sync with styleFlex[] in lexBase/lexerStyles.cpp */
#define STYLE_TEXT			0
#define STYLE_COMMENT		1
#define STYLE_DIRECTIVE		2
#define STYLE_DEFNAME		3
#define STYLE_PATTERN		4
#define STYLE_STRING		5
#define STYLE_CLASS			6
#define STYLE_REFERENCE		7
#define STYLE_STATECOND		8
#define STYLE_NUMBER		9
#define STYLE_CODE			10
#define STYLE_KEYWORD		11
#define STYLE_SECTION		12

/* yy_start keeps the start condition in its low 8 bits only.  The brace depth
   of the action block being highlighted and the state to resume after a
   comment are kept in the upper bits and serialized into the line state at
   every newline, so that styling can resume in the middle of the file. */
#define LINE_STATE \
	((yy_start & 0xFF) | ((braceDepth & 0xFF) << 8) | ((prevState & 0xFF) << 16))

#undef	YY_BREAK
#define YY_BREAK \
	if( yyleng > 0 && yytext[yyleng-1] == '\n' ) \
		m_pStyler->SetLineState( m_foldLine, LINE_STATE ); \
	break;
%}

%%
 /*************************************************************************************/

%{
	int extra = yy_start >> 8;
	yy_start &= 0xFF;						// the DFA only understands the start condition
	int braceDepth = extra & 0xFF;			// nesting of the current { ... } action block
	int prevState = (extra >> 8) & 0xFF;	// state to resume after a comment

	static const char* pszCKeywords =
		"auto break case char const continue default do double else enum extern float "
		"for goto if inline int long register restrict return short signed sizeof static "
		"struct switch typedef union unsigned void volatile while _Bool _Complex _Imaginary";
	CKeywords kwC( pszCKeywords, true );
%}

 /*************************************************************************************/
 /* definitions section - %{ ... %}, directives and name definitions
  *************************************************************************************/

<INITIAL>{
	^"%%"					SetStyle( STYLE_SECTION ); BEGIN( RULES );

	^{WS}*"%{"				SetStyle( STYLE_DIRECTIVE ); BEGIN( PROLOGD ); levelIncrease();
	^{WS}*"%"[A-Za-z_]+		SetStyle( STYLE_DIRECTIVE );
	^{WS}*"%"				SetStyle( STYLE_DIRECTIVE );

	 /* a word at the beginning of a line names a definition */
	^{NAME}					SetStyle( STYLE_DEFNAME );

	"/*"					prevState = YY_START; BEGIN( COMMENT ); SetStyle( STYLE_COMMENT );
	"//"[^\r\n]*			SetStyle( STYLE_COMMENT );

	\"([^"\\\r\n]|\\.)*\"	SetStyle( STYLE_STRING );
	\"						SetStyle( STYLE_STRING );
	"["("\\"[^\r\n]|[^\]\r\n])*"]"	SetStyle( STYLE_CLASS );

	"{"{NAME}"}"			SetStyle( STYLE_REFERENCE );
	"{"{DC}+","{DC}+"}"		SetStyle( STYLE_NUMBER );
	"{"{DC}+",}"			SetStyle( STYLE_NUMBER );
	"{"{DC}+"}"				SetStyle( STYLE_NUMBER );

	\\[^\r\n]				SetStyle( STYLE_PATTERN );
	[()|*+?.^$/]			SetStyle( STYLE_PATTERN );

	{WORD}					SetStyle( STYLE_TEXT );
	{NUM}					SetStyle( STYLE_TEXT );
}

 /*************************************************************************************/
 /* rules section - patterns, <start conditions> and their actions
  *************************************************************************************/

<RULES>{
	^"%%"					SetStyle( STYLE_SECTION ); BEGIN( CODE );
	^{WS}*"%{"				SetStyle( STYLE_DIRECTIVE ); BEGIN( PROLOGR ); levelIncrease();

	 /* indentation is not part of a pattern */
	^{WS}+					SetStyle( STYLE_TEXT );

	 /* <SC>{ ... } scope */
	"<"{SC_LIST}">""{"		{
		SetStyle( MatchStart(), MatchEnd()-1, STYLE_STATECOND );
		SetStyle( MatchEnd()-1, MatchEnd(), STYLE_PATTERN );
		levelIncrease();
	}
	^"}"					SetStyle( STYLE_PATTERN ); levelDecrease();

	 /* <<EOF>> and <start condition> lists */
	"<<"[^>\r\n]*">>"		SetStyle( STYLE_STATECOND );
	"<"{SC_LIST}">"			SetStyle( STYLE_STATECOND );

	 /* comments */
	"/*"					prevState = YY_START; BEGIN( COMMENT ); SetStyle( STYLE_COMMENT );
	"//"[^\r\n]*			SetStyle( STYLE_COMMENT );

	 /* the pattern ends at the first whitespace, the action follows it */
	{WS}+"{"				{
		SetStyle( MatchStart(), MatchEnd()-1, STYLE_TEXT );
		SetStyle( MatchEnd()-1, MatchEnd(), STYLE_CODE );
		braceDepth = 1;
		BEGIN( ACTION );
		levelIncrease();
	}
	{WS}+					BEGIN( ACTION ); braceDepth = 0; SetStyle( STYLE_TEXT );

	\"([^"\\\r\n]|\\.)*\"	SetStyle( STYLE_STRING );
	\"						SetStyle( STYLE_STRING );
	"["("\\"[^\r\n]|[^\]\r\n])*"]"	SetStyle( STYLE_CLASS );

	"{"{NAME}"}"			SetStyle( STYLE_REFERENCE );
	"{"{DC}+","{DC}+"}"		SetStyle( STYLE_NUMBER );
	"{"{DC}+",}"			SetStyle( STYLE_NUMBER );
	"{"{DC}+"}"				SetStyle( STYLE_NUMBER );

	\\[^\r\n]				SetStyle( STYLE_PATTERN );
	[()|*+?.^$]				SetStyle( STYLE_PATTERN );

	{WORD}					SetStyle( STYLE_TEXT );
	{NUM}					SetStyle( STYLE_TEXT );
}

 /*************************************************************************************/
 /* C code copied verbatim - %{ ... %} prologue, action blocks and user code
  *************************************************************************************/

<PROLOGD>{
	^{WS}*"%}"				SetStyle( STYLE_DIRECTIVE ); BEGIN( INITIAL ); levelDecrease();
}

<PROLOGR>{
	^{WS}*"%}"				SetStyle( STYLE_DIRECTIVE ); BEGIN( RULES ); levelDecrease();
}

<ACTION>{
	{EOL}	{
		if( braceDepth == 0 ) {
			BEGIN( RULES );
			SetStyle( STYLE_TEXT );
		} else
			SetStyle( STYLE_CODE );
	}
	"{"		{
		if( braceDepth > 0 ) {
			braceDepth++;
			levelIncrease();
		}
		SetStyle( STYLE_CODE );
	}
	"}"		{
		if( braceDepth > 0 ) {
			levelDecrease();
			if( --braceDepth == 0 )
				BEGIN( RULES );
		}
		SetStyle( STYLE_CODE );
	}
}

<PROLOGD,PROLOGR,ACTION,CODE>{
	"/*"					prevState = YY_START; BEGIN( COMMENT ); SetStyle( STYLE_COMMENT );
	"//"[^\r\n]*			SetStyle( STYLE_COMMENT );
	\"([^"\\\r\n]|\\.)*\"	SetStyle( STYLE_STRING );
	\'([^'\\\r\n]|\\.)*\'	SetStyle( STYLE_STRING );
	^{WS}*"#"[^\r\n]*		SetStyle( STYLE_DIRECTIVE );
	{NUM}					SetStyle( STYLE_NUMBER );
	{NAME}					SetStyle( kwC.is(yytext) ? STYLE_KEYWORD : STYLE_CODE );
}

 /*************************************************************************************/
 /* comment
  *************************************************************************************/

<COMMENT>{
	"*/"					SetStyle( STYLE_COMMENT ); BEGIN( prevState );
}

 /*************************************************************************************/
 /* default rule
  *************************************************************************************/

<*>{
	{EOL}	|
	{WS}+	|
	.		SetStyle();
}

%%
 /*************************************************************************************/

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case INITIAL:	return STYLE_TEXT;
		case RULES:		return STYLE_TEXT;
		case PROLOGD:	return STYLE_CODE;
		case PROLOGR:	return STYLE_CODE;
		case ACTION:	return STYLE_CODE;
		case CODE:		return STYLE_CODE;
		case COMMENT:	return STYLE_COMMENT;
	}
	return STYLE_TEXT;
}
