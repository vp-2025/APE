%{
/******************************************************************************
* 		plain text documents (*.txt)
*
*   Title            a line underlined by === or --- ,
*   =====            a line in CAPITALS or a line ending with ':' is a heading;
*   NOTES:           the text folds from one heading to the next
*
*   - item  * item  + item  bullet item      list markers at the line start,
*   1. item  2) item  1.2. item  a) item     also numbered ones
*
*   # comment  // comment  ; comment          comment lines
*
*   2026-10-29  29.10.2026  10/29/2026        dates
*   12:30  12:30:05  12:30:05.250             times
*   user@host.com                             e-mail addresses
*   http://host/path  https://...  www....    links (click to open)
*   "quoted text"  42  3.14  0x1F  <>()[]...  strings, numbers and symbols
******************************************************************************/
#include "LexBase.h"
%}

%option  c++
%option  prefix="text_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x STRING

SWC		[A-Za-z_\x80-\xFF]
WWC		[A-Za-z_0-9\x80-\xFF]
DC		[0-9]
HC		[0-9A-Fa-f]
EOL		(\r?\n)
WS		[ \t]
WORD	{SWC}{WWC}*
BULLET	([-*+]|"\xE2\x80\xA2")
NUMBERED	({DC}{1,3}("."{DC}{1,3})*[.)]|[A-Za-z][)])
CAPS	[A-Z0-9 .,&/()'_:-]

%{
/* style ids - keep in sync with styleText[] in lexBase/lexerStyles.cpp */
#define STYLE_TEXT			0
#define STYLE_SYMBOL		1
#define STYLE_NUMBER		2
#define STYLE_STRING		3
#define STYLE_UNQ			5
#define STYLE_LINK			6
#define STYLE_COMMENT		7
#define STYLE_HEADING		8
#define STYLE_LIST			9
#define STYLE_DATE			10
#define STYLE_TIME			11
#define STYLE_EMAIL			12
#define STYLE_EOL			13		/* line ends - light grey, visible with "show line endings" */
%}

%%
 /*************************************************************************************/

%{
	// a heading starts a new fold section - it closes the previous one
	auto heading = [&]() {
		SetStyle( STYLE_HEADING );
		if( GetLevel() )
			levelDecrease();
		levelIncrease();
	};
%}

<INITIAL>{
	 /* comment lines */
	^{WS}*("#"|"//"|";")[^\r\n]*		SetStyle( STYLE_COMMENT );

	 /* headings: a title line underlined by the next one ... */
	^[^ \t\r\n=-][^\r\n]*/"\n==="		heading();
	^[^ \t\r\n=-][^\r\n]*/"\r\n==="		heading();
	^[^ \t\r\n=-][^\r\n]*/"\n---"		heading();
	^[^ \t\r\n=-][^\r\n]*/"\r\n---"		heading();
	^(={3,}|-{3,}){WS}*/[\r\n]			SetStyle( STYLE_HEADING );

	 /* ... a line in capitals or a line ending with ':' */
	^[A-Z]{CAPS}*[A-Z]{CAPS}*/[\r\n]		heading();
	^[A-Za-z\x80-\xFF][^\r\n]*":"{WS}*/[\r\n]	heading();

	 /* list markers */
	^{WS}*({BULLET}|{NUMBERED})/{WS}	SetStyle( STYLE_LIST );

	 /* dates and times */
	{DC}{4}"-"{DC}{2}"-"{DC}{2}				|
	{DC}{1,2}"."{DC}{1,2}"."{DC}{2,4}		|
	{DC}{1,2}"/"{DC}{1,2}"/"{DC}{2,4}		SetStyle( STYLE_DATE );
	{DC}{1,2}":"{DC}{2}(":"{DC}{2}([.,]{DC}+)?)?	SetStyle( STYLE_TIME );

	 /* links and e-mail addresses */
	([hH][tT][tT][pP][sS]?"://"|[wW][wW][wW]".")[A-Za-z0-9?&%_.+=#/:~-]+	SetStyle( STYLE_LINK );
	[A-Za-z0-9._%+-]+"@"[A-Za-z0-9-]+("."[A-Za-z0-9-]+)+	SetStyle( STYLE_EMAIL );

	\"						{
		if( isQuote() ) {
			BEGIN( STRING );
			SetStyle( STYLE_STRING );
		} else
			SetStyle( STYLE_SYMBOL );
	}

	 /* numbers - "3rd", "2x" and the like are words */
	0[xX]{HC}+				|
	{DC}+("."{DC}+)?		SetStyle( STYLE_NUMBER );
	{DC}+{WORD}				SetStyle( STYLE_TEXT );

	{WORD}					SetStyle( STYLE_TEXT );

	[<>(){}\[\]+*=&|~`'/\\,.:;%@?!#$^-]	SetStyle( STYLE_SYMBOL );
}

<STRING>{
	\"						SetStyle(); BEGIN( INITIAL );
	[^"\r\n]+				SetStyle();
	 /* the string is not closed at the end of the line */
	{EOL}					SetStyle( STYLE_UNQ ); BEGIN( INITIAL );
}

<*>{
	{EOL}					SetStyle( STYLE_EOL );
	{WS}+	|
	.						SetStyle();
}

%%
/*************************************************************************************/

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case STRING:	return STYLE_STRING;
	}
	return STYLE_TEXT;
}
