%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="yaml_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x FLOW DQ SQ BLOCKHDR BLOCK

EOL		(\r?\n)
WS		[ \t]
 /* block context plain scalar: first word can't start with an indicator, the next ones can't start with '#' (comment) */
PF		([^ \t\r\n\-?:,\[\]{}#&*!|>'"%@`]|[-?:][^ \t\r\n])
PC		([^ \t\r\n:]|:+[^ \t\r\n:])
PWN		([^ \t\r\n:#]|:+[^ \t\r\n:]){PC}*
PLAIN	{PF}{PC}*({WS}+{PWN})*
 /* flow context plain scalar: additionally stops at , [ ] { } */
FF		([^ \t\r\n\-?:,\[\]{}#&*!|>'"%@`]|[-?:][^ \t\r\n,\[\]{}])
FC		([^ \t\r\n:,\[\]{}]|:+[^ \t\r\n:,\[\]{}])
FWN		([^ \t\r\n:#,\[\]{}]|:+[^ \t\r\n:,\[\]{}]){FC}*
FPLAIN	{FF}{FC}*({WS}+{FWN})*

DQSTR	\"([^"\\\r\n]|\\.)*\"
SQSTR	'([^'\r\n]|'')*'

INT		[-+]?([0-9][0-9_]*|0x[0-9a-fA-F_]+|0o[0-7_]+|0b[01_]+)
FLOAT	([-+]?[0-9][0-9_]*)?\.[0-9_]+([eE][-+]?[0-9]+)?|[-+]?[0-9][0-9_]*\.?[eE][-+]?[0-9]+|[-+]?[0-9][0-9_]*\.|[-+]?\.(inf|Inf|INF)|\.(nan|NaN|NAN)
DATE	[0-9]{4}-[0-9]{1,2}-[0-9]{1,2}(([Tt]|{WS}+)[0-9]{1,2}:[0-9]{2}:[0-9]{2}(\.[0-9]*)?({WS}*(Z|[-+][0-9]{1,2}(:[0-9]{2})?))?)?
KW		(true|True|TRUE|false|False|FALSE|yes|Yes|YES|no|No|NO|on|On|ON|off|Off|OFF|null|Null|NULL|~)

%{
/* style ids - keep in sync with styleYAML[] in lexBase/lexerStyles.cpp */
#define STYLE_TEXT			0
#define STYLE_COMMENT		1
#define STYLE_KEY			2
#define STYLE_STRING		3
#define STYLE_STRING2		4
#define STYLE_NUMBER		5
#define STYLE_KEYWORD		6
#define STYLE_SYMBOL		7
#define STYLE_DOCUMENT		8
#define STYLE_ANCHOR		9
#define STYLE_TAG			10
#define STYLE_DIRECTIVE		11
#define STYLE_BLOCK			12

/* YAML folds by indentation, which the brace-counting levelIncrease()/levelDecrease()
   can't express: every rule first goes through yamlBeforeAction(), which sets the fold
   level of the line from the indentation of its first token and turns the previous
   content line into a fold header when the level grows. */
#undef	YY_USER_ACTION
#define YY_USER_ACTION \
	if( yy_act != YY_END_OF_BUFFER ) { \
		yamlBeforeAction(); \
		this->BeforeAction(yytext, yyleng); \
	}

/* the line state keeps the block scalar indent and the flow depth besides the start
   condition, they are restored at the start of yylex() */
#undef	YY_BREAK
#define YY_BREAK \
	if( yyleng > 0 && yytext[yyleng-1] == '\n' ) \
		m_pStyler->SetLineState( m_foldLine, yamlLineState() ); \
	break;

#define FLOW_OR_INITIAL		(flowDepth ? FLOW : INITIAL)
%}

%%
 /*************************************************************************************/

%{
	int extra = yy_start >> 8;
	yy_start &= 0xFF;
	int blockIndent = (extra & 0xFFF) - 2;	// a block scalar line must be indented deeper than this
	int flowDepth = (extra >> 12) & 0xFF;	// [ ] { } nesting

	int lineStart = -1;			// position of the current line start
	int lineIndent = 0;			// indent of the first token of the current line
	int keyCol = -1;			// column of the last key on the current line
	bool lineDone = false;		// fold level of the current line is already set
	bool docLine = false;		// the current line starts with ---
	int prevContent = -1;		// last line with a fold level of its own (not blank / comment)

	auto setFold = [&]( int units, bool bContent ) {
		if( units > 0xB00 ) units = 0xB00;
		int level = SC_FOLDLEVELBASE + units;
		if( level > m_foldLevel && prevContent >= 0 && prevContent < m_foldLine ) {
			m_pStyler->SetLevel( prevContent, (m_pStyler->GetLevel(prevContent) & SC_FOLDLEVELNUMBERMASK) | SC_FOLDLEVELHEADERFLAG );
			for( int line = prevContent+1; line < m_foldLine; line++ )
				m_pStyler->SetLevel( line, level );
		}
		m_foldLevel = level;
		if( bContent )
			prevContent = m_foldLine;
	};

	auto yamlBeforeAction = [&]() {
		if( lineStart < 0 )
			lineStart = MatchStart();	// styling always starts at a line start
		if( yyleng <= 0 )
			return;
		if( yytext[yyleng-1] == '\n' ) {
			if( YY_START == BLOCK && !lineDone )	// blank lines belong to the block scalar
				setFold( (blockIndent+2)*2, false );
			lineStart = MatchEnd();
			lineDone = false;
			docLine = false;
			keyCol = -1;
			return;
		}
		if( lineDone || YY_START == BLOCK )		// the BLOCK rules set the fold themselves
			return;
		int n = 0;
		while( n < yyleng && (yytext[n] == ' ' || yytext[n] == '\t') )
			n++;
		if( n == yyleng )
			return;
		lineDone = true;
		lineIndent = MatchStart() + n - lineStart;
		if( yytext[n] == '#' )		// comment only line: keep the level of the previous line
			return;
		bool bDash = yytext[n] == '-' && yyleng-n == 1;	// "- item": deeper than its parent key
		setFold( lineIndent*2 + (bDash ? 1 : 0), true );
	};

	auto yamlLineState = [&]() {
		int ext = 0;
		if( YY_START == BLOCK ) {
			int indent = blockIndent+2;
			ext = indent < 0 ? 0 : indent > 0xFFF ? 0xFFF : indent;
		}
		ext |= (flowDepth > 0xFF ? 0xFF : flowDepth) << 12;
		return yy_start | (ext << 8);
	};

	auto isDocMarker = [&]() {
		return yyleng >= 3 &&
			( (yytext[0] == '-' && yytext[1] == '-' && yytext[2] == '-') ||
			  (yytext[0] == '.' && yytext[1] == '.' && yytext[2] == '.') ) &&
			( yyleng == 3 || yytext[3] == ' ' || yytext[3] == '\t' );
	};

	auto setDocMarker = [&]() {
		flowDepth = 0;
		docLine = true;
		BEGIN(INITIAL);
		SetStyle( STYLE_DOCUMENT );
	};

	auto setKey = [&]() {
		keyCol = MatchStart() - lineStart;
		SetStyle( MatchStart(), MatchEnd()-1, STYLE_KEY );
		SetStyle( MatchEnd()-1, MatchEnd(), STYLE_SYMBOL );
	};
%}

 /* document start / end, directives */
<INITIAL,FLOW>^(---|\.\.\.)/[ \t\r\n]	setDocMarker();
^%[^\r\n]*					SetStyle( STYLE_DIRECTIVE );

<INITIAL,FLOW,BLOCKHDR>#[^\r\n]*	SetStyle( STYLE_COMMENT );

 /* keys */
{PLAIN}{WS}*:/[ \t\r\n]		setKey();
<FLOW>{FPLAIN}{WS}*:/[ \t\r\n,\[\]{}]	setKey();
<INITIAL,FLOW>{DQSTR}{WS}*:	|
<INITIAL,FLOW>{SQSTR}{WS}*:	setKey();

<INITIAL,FLOW>{
 /* scalars */
	{DQSTR}					SetStyle( STYLE_STRING );
	\"([^"\\\r\n]|\\.)*\\?	BEGIN(DQ); SetStyle( STYLE_STRING );
	{SQSTR}					SetStyle( STYLE_STRING2 );
	'([^'\r\n]|'')*			BEGIN(SQ); SetStyle( STYLE_STRING2 );

	{INT}	|
	{FLOAT}	|
	{DATE}					SetStyle( STYLE_NUMBER );
	{KW}					SetStyle( STYLE_KEYWORD );

	!<[^>\r\n]*>	|
	![^ \t\r\n,\[\]{}]*		SetStyle( STYLE_TAG );
	[&*][^ \t\r\n,\[\]{}]+	SetStyle( STYLE_ANCHOR );

 /* flow collections */
	[\[{]					{
		flowDepth++;
		BEGIN(FLOW);
		SetStyle( STYLE_SYMBOL );
	}
	[\]}]					{
		if( flowDepth > 0 ) flowDepth--;
		BEGIN(FLOW_OR_INITIAL);
		SetStyle( STYLE_SYMBOL );
	}
}

<INITIAL>{
	{PLAIN}					{
		// "--- |", "--- text": the longer plain scalar wins over the ^--- rule
		if( MatchStart() == lineStart && isDocMarker() ) {
			yyless(3);
			setDocMarker();
		} else
			SetStyle( STYLE_TEXT );
	}
	[-?:]/[ \t\r\n]			SetStyle( STYLE_SYMBOL );

 /* block scalar header: | |- |+ |2 > >- ... */
	[|>]([-+][1-9]?|[1-9][-+]?)?/[ \t\r\n]	{
		if( docLine )
			blockIndent = -1;
		else
			blockIndent = keyCol >= 0 ? keyCol : lineIndent;
		BEGIN(BLOCKHDR);
		SetStyle( STYLE_SYMBOL );
	}
}

<FLOW>{
	{FPLAIN}				SetStyle( STYLE_TEXT );
	[?:]/[ \t\r\n,\[\]{}]	SetStyle( STYLE_SYMBOL );
	,						SetStyle( STYLE_SYMBOL );
}

<DQ>{
	([^"\\\r\n]|\\.)*\"		BEGIN(FLOW_OR_INITIAL); SetStyle( STYLE_STRING );
	([^"\\\r\n]|\\.)+\\?	|
	\\						SetStyle( STYLE_STRING );
}

<SQ>{
	([^'\r\n]|'')*'			BEGIN(FLOW_OR_INITIAL); SetStyle( STYLE_STRING2 );
	([^'\r\n]|'')+			SetStyle( STYLE_STRING2 );
}

<BLOCKHDR>{EOL}				BEGIN(BLOCK); SetStyle( STYLE_TEXT );

 /* block scalar body: lasts while the lines are indented deeper than the parent node */
<BLOCK>{
	{WS}*{EOL}				SetStyle( STYLE_BLOCK );
	[ ]*[^ \r\n][^\r\n]*	{
		int ind = 0;
		while( yytext[ind] == ' ' )
			ind++;
		if( ind > blockIndent && !(ind == 0 && isDocMarker()) ) {
			lineDone = true;
			setFold( (blockIndent+2)*2, true );
			if( ind )
				SetStyle( MatchStart(), MatchStart()+ind, STYLE_TEXT );
			SetStyle( MatchStart()+ind, MatchEnd(), STYLE_BLOCK );
		} else {
			// the block is over - rescan the line as a regular one
			BEGIN(INITIAL);
			yyless(0);
			yy_current_buffer->yy_at_bol = 1;
		}
	}
}

<*>{
	{EOL}	|
	{WS}+	|
	.		SetStyle();
}

%%
/*************************************************************************************/

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case DQ:		return STYLE_STRING;
		case SQ:		return STYLE_STRING2;
		case BLOCK:		return STYLE_BLOCK;
	}
	return STYLE_TEXT;
}
