%{
/******************************************************************************
* 		JSON files (*.json) - also JSON5 / JSONC extras
*
*   { "key": "value", 'key': 'value', key: value }    keys and values styled apart
*   "tab\t new line\n \u00e9 \u{1F600} \x41"      escape sequences
*   42  -3.5  1e10  .5  0x1F                          numbers
*   true false null NaN Infinity                      keywords
*   // comment  /\* comment *\/                         comments (JSONC / JSON5)
*
* A key is a string or a word followed by ':' that comes after '{' or ',' -
* comments between them do not matter.
* { } and [ ] fold.
******************************************************************************/
#include "LexBase.h"
%}

%option  c++
%option  prefix="json_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x COMMENT STRING SQSTRING

SWC		[A-Za-z_$]
WWC		[A-Za-z_$0-9]
DC		[0-9]
HC		[0-9A-Fa-f]
EOL		(\r?\n)
WS		[ \t]
WORD	{SWC}{WWC}*

%{
/* style ids - keep in sync with styleJSON[] in lexBase/lexerStyles.cpp */
#define STYLE_TEXT			0
#define STYLE_KEYWORD		1
#define STYLE_COMMENT		2
#define STYLE_SYMBOL		3
#define STYLE_STRING		4
#define STYLE_SQSTRING		5
#define STYLE_NUMBER		6
#define STYLE_UNQ			8
#define STYLE_KEY			13
#define STYLE_ESCAPE		14
#define STYLE_EOL			15

/* yy_start keeps the start condition in its low 8 bits only.  Whether a key
   may come next is kept above them and serialized into the line state at
   every newline, so that styling can resume in the middle of the file. */
#define LINE_STATE			((yy_start & 0xFF) | (bNoKey ? 0x100 : 0))

#undef	YY_BREAK
#define YY_BREAK \
	if( yyleng > 0 && yytext[yyleng-1] == '\n' ) \
		m_pStyler->SetLineState( m_foldLine, LINE_STATE ); \
	break;
%}

%%

%{
	static CKeywords kwJSON( "true false null NaN Infinity", true );

	// a key may follow "{" or "," only - set by every token but comments and spaces
	bool bNoKey = (yy_start & 0x100) != 0;
	yy_start &= 0xFF;						// the DFA only understands the start condition
%}

<INITIAL>{
	 /* keys - "key":  'key':  key:  the whitespace and ':' styled apart */
	\"([^"\\\r\n]|\\.)*\"{WS}*":"	|
	'([^'\\\r\n]|\\.)*'{WS}*":"		|
	{WORD}{WS}*":"				{
		bool bKey = !bNoKey;
		bNoKey = true;						// a value follows either way
		if( bKey ) {
			int n = yyleng-1, e = n;			// the ':' and the end of the key
			while( e > 0 && (yytext[e-1] == ' ' || yytext[e-1] == '\t') )
				e--;
			SetStyle( MatchStart(), MatchStart()+e, STYLE_KEY );
			SetStyle( MatchStart()+e, MatchStart()+n, STYLE_TEXT );
			SetStyle( MatchStart()+n, MatchEnd(), STYLE_SYMBOL );
		} else if( *yytext == '"' ) {		// not a key: a plain string or word
			yyless( 1 ); BEGIN( STRING ); SetStyle();
		} else if( *yytext == '\'' ) {
			yyless( 1 ); BEGIN( SQSTRING ); SetStyle();
		} else {
			int w = 0;
			while( isalnum((unsigned char)yytext[w]) || yytext[w] == '_' || yytext[w] == '$' )
				w++;
			yyless( w );
			SetStyle( kwJSON.is(yytext) ? STYLE_KEYWORD : STYLE_TEXT );
		}
	}

	\"						BEGIN( STRING ); SetStyle(); bNoKey = true;
	'						BEGIN( SQSTRING ); SetStyle(); bNoKey = true;

	"//"[^\r\n]*			SetStyle( STYLE_COMMENT );
	"/*"					BEGIN( COMMENT ); SetStyle(); levelIncrease();

	{WORD}					SetStyle( kwJSON.is(yytext) ? STYLE_KEYWORD : STYLE_TEXT ); bNoKey = true;

	 /* numbers - the sign is a symbol */
	0[xX]{HC}+				|
	{DC}+("."{DC}*)?([eE][+-]?{DC}+)?	|
	"."{DC}+([eE][+-]?{DC}+)?	SetStyle( STYLE_NUMBER ); bNoKey = true;

	 /* folding */
	[{\[]					SetStyle( STYLE_SYMBOL ); levelIncrease(); bNoKey = *yytext == '[';
	[}\]]					SetStyle( STYLE_SYMBOL ); levelDecrease(); bNoKey = true;

	[:,+\-.]				SetStyle( STYLE_SYMBOL ); bNoKey = *yytext != ',';

	 /* non-ASCII outside of a string */
	[\x80-\xFF]+			SetStyle( IStyler::styleInvalidText ); bNoKey = true;
}

<COMMENT>{
	"*/"					SetStyle(); BEGIN( INITIAL ); levelDecrease();
	[^*\r\n]+				SetStyle();
}

<STRING>{
	\"						SetStyle(); BEGIN( INITIAL );
	[^"\\\r\n]+				SetStyle();
}

<SQSTRING>{
	'						SetStyle(); BEGIN( INITIAL );
	[^'\\\r\n]+				SetStyle();
}

<STRING,SQSTRING>{
	 /* escape sequences, a '\' at the line end continues the string (JSON5) */
	\\[bfnrtv0'"/\\]		|
	\\u{HC}{4}				|
	\\u"{"{HC}+"}"			|
	\\x{HC}{2}				SetStyle( STYLE_ESCAPE );

	 /* an invalid escape sequence */
	\\.						SetStyle( IStyler::styleInvalidText );

	 /* the string is not closed at the end of the line */
	{EOL}					SetStyle( STYLE_UNQ ); BEGIN( INITIAL );
}

<*>{
	{EOL}					SetStyle( STYLE_EOL );
	{WS}+	|
	.						SetStyle();
}

%%

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case COMMENT:	return STYLE_COMMENT;
		case STRING:	return STYLE_STRING;
		case SQSTRING:	return STYLE_SQSTRING;
	}
	return STYLE_TEXT;
}
