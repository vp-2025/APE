%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="md_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x FENCE
%x FRONT
%x FENCEEND

EOL			(\r?\n)
SPACE		[ \t]
SWC			[A-Za-z_]
WWC			[_A-Za-z0-9]
WORD		{SWC}{WWC}*
URLCH		[^ \t\r\n<>()\[\]]

%{
/* style ids - keep in sync with styleMD[] in lexBase/lexerStyles.cpp */
#define STYLE_TEXT			0
#define STYLE_H1			1
#define STYLE_H2			2
#define STYLE_H3			3
#define STYLE_H4			4
#define STYLE_H5			5
#define STYLE_H6			6
#define STYLE_BOLD			7
#define STYLE_ITALIC		8
#define STYLE_BOLDITALIC	9
#define STYLE_STRIKE		10
#define STYLE_CODE			11
#define STYLE_CODEBLOCK		12
#define STYLE_LINK			13
#define STYLE_URL			14
#define STYLE_LIST			15
#define STYLE_QUOTE			16
#define STYLE_HR			17
#define STYLE_TABLE			18
#define STYLE_HTML			19
#define STYLE_ESCAPE		20
#define STYLE_TASK			21
#define STYLE_MARK			22
#define STYLE_REF			23
#define STYLE_FRONT			24

/* style a span, putting its leading/trailing delimiters into STYLE_MARK */
#define STYLE_SPAN(nPre,nPost,nStyle) \
	do { \
		if( nPre ) \
			SetStyle( MatchStart(), MatchStart()+(nPre), STYLE_MARK ); \
		if( MatchEnd()-(nPost) > MatchStart()+(nPre) ) \
			SetStyle( MatchStart()+(nPre), MatchEnd()-(nPost), (nStyle) ); \
		if( nPost ) \
			SetStyle( MatchEnd()-(nPost), MatchEnd(), STYLE_MARK ); \
	} while( 0 )
%}

%%
 /*************************************************************************************/

 /* YAML front matter: only valid as the very first line of the file.
    A "---" anywhere else is a horizontal rule. */
^---{SPACE}*{EOL}	{
		if( MatchStart()==0 ) {
			BEGIN(FRONT);
			SetStyle( STYLE_FRONT );
		} else
			SetStyle( STYLE_HR );
	}

 /* horizontal rule: ***, ___, - - -, * * * */
^([-*_]{SPACE}*){3,}{EOL}	SetStyle( STYLE_HR );

 /* ATX heading: # .. ###### (needs a space, so #tag stays text) */
^#{1,6}{SPACE}[^\n]* |
^#{1,6}{SPACE}*{EOL}	{
		int nLevel = 0;
		while( yytext[nLevel]=='#' ) nLevel++;
		SetStyle( MatchStart(), MatchStart()+nLevel, STYLE_MARK );
		SetStyle( MatchStart()+nLevel, MatchEnd(), STYLE_H1+nLevel-1 );
	}

 /* setext heading underline: === (the --- case is handled above) */
^={2,}{SPACE}*{EOL}	SetStyle( STYLE_H1 );

 /* task list item: - [ ] / - [x] */
^[ \t]{0,3}([-*+]|[0-9]{1,9}[.)]){SPACE}+\[[ xX]\]{SPACE}+	{
		int nChr = 0;
		while( yytext[nChr]==' ' || yytext[nChr]=='\t' ) nChr++;
		while( yytext[nChr]=='-' || yytext[nChr]=='*' || yytext[nChr]=='+' ||
				(yytext[nChr]>='0' && yytext[nChr]<='9') ||
				yytext[nChr]=='.' || yytext[nChr]==')' )
			nChr++;
		SetStyle( MatchStart(), MatchStart()+nChr, STYLE_LIST );
		SetStyle( MatchStart()+nChr, MatchEnd(), STYLE_TASK );
	}

 /* list item marker: -, *, +, 1. , 1) */
^[ \t]{0,3}([-*+]|[0-9]{1,9}[.)]){SPACE}+ |
^[ \t]{0,3}([-*+]|[0-9]{1,9}[.)]){SPACE}*{EOL}	SetStyle( STYLE_LIST );

 /* blockquote marker */
^[ \t]{0,3}>+{SPACE}?	SetStyle( STYLE_QUOTE );

 /* table delimiter row: |---|:--:|---| */
^[ \t]*\|?{SPACE}*:?-+:?{SPACE}*(\|{SPACE}*:?-+:?{SPACE}*)+(\|{SPACE}*)?{EOL}	SetStyle( STYLE_TABLE );

 /* link reference definition: [label]: url */
^[ \t]{0,3}\[[^\]\n]+\]:[^\n]*	{
		int nClose = 0;
		while( yytext[nClose] && yytext[nClose]!=']' ) nClose++;
		SetStyle( MatchStart(), MatchStart()+nClose+2, STYLE_REF );
		SetStyle( MatchStart()+nClose+2, MatchEnd(), STYLE_URL );
	}

 /* fenced code block: ``` or ~~~.
    NOTE: no {EOL} here - the fold level a rule sets is applied when the line
    end is matched separately (see CSciLexer::BeforeAction). */
^[ \t]{0,3}(`{3,}|~{3,})[^\n]*	{
		levelIncrease();
		BEGIN(FENCE);
		SetStyle( STYLE_CODEBLOCK );
	}

<FENCE>^[ \t]{0,3}(`{3,}|~{3,}){SPACE}*	{
		levelDecrease();
		BEGIN(FENCEEND);
		SetStyle( STYLE_CODEBLOCK );
	}

<FENCEEND>{EOL}	{
		BEGIN(INITIAL);
		SetStyle( STYLE_CODEBLOCK );
	}

 /* end of front matter */
<FRONT>^---{SPACE}*{EOL} |
<FRONT>^\.\.\.{SPACE}*{EOL}	{
		BEGIN(INITIAL);
		SetStyle( STYLE_FRONT );
	}

 /* html comment */
(<!--[^\n]*-->)				SetStyle( STYLE_HTML );

 /* autolink: <scheme:...> */
(<[A-Za-z][A-Za-z0-9+.\-]*:[^<>\n]*>)	SetStyle( STYLE_URL );

 /* html tag */
(<![A-Za-z][^<>\n]*>) |
(<\/?[A-Za-z][^<>\n]*>)		SetStyle( STYLE_HTML );

 /* footnote reference: [^1] */
\[\^[^\]\n]+\]				SetStyle( STYLE_REF );

 /* links & images: [text](url), [text][ref] */
!?\[[^\]\n]*\]\([^)\n]*\) |
!?\[[^\]\n]*\]\[[^\]\n]*\]	{
		int nStart = (yytext[0]=='!') ? 1 : 0;
		int nClose = nStart;
		if( nStart )
			SetStyle( MatchStart(), MatchStart()+1, STYLE_MARK );
		while( nClose<yyleng && yytext[nClose]!=']' ) nClose++;
		SetStyle( MatchStart()+nStart, MatchStart()+nStart+1, STYLE_MARK );
		SetStyle( MatchStart()+nStart+1, MatchStart()+nClose, STYLE_LINK );
		int nAfter = MatchStart()+nClose+1;
		SetStyle( MatchStart()+nClose, nAfter, STYLE_MARK );
		if( nAfter<MatchEnd() ) {
			SetStyle( nAfter, nAfter+1, STYLE_MARK );
			SetStyle( nAfter+1, MatchEnd()-1, STYLE_URL );
			SetStyle( MatchEnd()-1, MatchEnd(), STYLE_MARK );
		}
	}

 /* bare url */
(https?|ftp|file):\/\/{URLCH}+	SetStyle( STYLE_URL );
www\.{URLCH}+					SetStyle( STYLE_URL );
mailto:{URLCH}+					SetStyle( STYLE_URL );

 /* inline code */
`+[^`\n]*`+	{
		int nPre = 0;
		while( nPre<yyleng && yytext[nPre]=='`' ) nPre++;
		int nEnd = yyleng;
		while( nEnd>0 && yytext[nEnd-1]=='`' ) nEnd--;
		STYLE_SPAN( nPre, yyleng-nEnd, STYLE_CODE );
	}

 /* emphasis */
\*\*\*[^ *\n][^*\n]*\*\*\*	STYLE_SPAN( 3, 3, STYLE_BOLDITALIC );
\*\*[^ *\n][^*\n]*\*\*		STYLE_SPAN( 2, 2, STYLE_BOLD );
\*[^ *\n][^*\n]*\*			STYLE_SPAN( 1, 1, STYLE_ITALIC );
___[^ _\n][^_\n]*___		STYLE_SPAN( 3, 3, STYLE_BOLDITALIC );
__[^ _\n][^_\n]*__			STYLE_SPAN( 2, 2, STYLE_BOLD );
_[^ _\n][^_\n]*_			STYLE_SPAN( 1, 1, STYLE_ITALIC );
~~[^ ~\n][^~\n]*~~			STYLE_SPAN( 2, 2, STYLE_STRIKE );

 /* escaped char */
\\[\\`*_{}\[\]()#+.!>~|\^\/-]	SetStyle( STYLE_ESCAPE );

 /* html entity */
&[A-Za-z][A-Za-z0-9]{1,31}; |
&#[0-9]{1,7}; |
&#[xX][0-9A-Fa-f]{1,6};		SetStyle( STYLE_ESCAPE );

 /* table cell separator */
\|							SetStyle( STYLE_TABLE );

<*>{
	{EOL} |
	{WORD} |
	{SPACE}+ |
	.						SetStyle();
}

%%
 /*************************************************************************************/

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case FENCE:		return STYLE_CODEBLOCK;
		case FENCEEND:	return STYLE_CODEBLOCK;
		case FRONT:		return STYLE_FRONT;
	}
	return STYLE_TEXT;
}
