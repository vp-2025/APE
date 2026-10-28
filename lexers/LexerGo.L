%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="go_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x COMMENT STRING RUNE RAWSTRING

SWC		[A-Za-z_\x80-\xFF]
WWC		[A-Za-z_0-9\x80-\xFF]
DC		[0-9]
HC		[0-9A-Fa-f]
OC		[0-7]
EOL		(\r?\n)
WS		[ \t]
WORD	{SWC}{WWC}*
DECS	{DC}(_?{DC})*
HEXS	{HC}(_?{HC})*
DEXP	([eE][+-]?{DECS})
HEXP	([pP][+-]?{DECS})
SYMB	[-+*/%&|^<>=!:.,;~\[\]]

%{

#define STYLE_TEXT			0
#define STYLE_KEYWORD		1
#define STYLE_COMMENT		2
#define STYLE_SYMBOL		3
#define STYLE_STRING		4
#define STYLE_RUNE			5
#define STYLE_NUMBER		6
#define STYLE_DIRECTIVE		7
#define STYLE_UNQ			8
#define STYLE_BUILTIN		9
#define STYLE_RAWSTRING		10

static CKeywords kwGo(
	"break case chan const continue default defer else fallthrough for func go goto "
	"if import interface map package range return select struct switch type var", true );
static CKeywords kwBuiltin(
	// types
	"any bool byte comparable complex64 complex128 error float32 float64 "
	"int int8 int16 int32 int64 rune string uint uint8 uint16 uint32 uint64 uintptr "
	// constants and the zero value
	"true false iota nil "
	// functions
	"append cap clear close complex copy delete imag len make max min new panic "
	"print println real recover", true );
%}

%%

<INITIAL>{
	 /* //go:build, //go:generate, //go:embed ... and the old "// +build" */
	"//go:"[a-z]+[^\r\n]*	|
	"//"{WS}*"+build"[^\r\n]*	SetStyle( STYLE_DIRECTIVE );

	"//"[^\r\n]*			SetStyle( STYLE_COMMENT );
	"/*"					BEGIN( COMMENT ); SetStyle( STYLE_COMMENT ); levelIncrease();

	\"						BEGIN( STRING ); SetStyle( STYLE_STRING );
	'						BEGIN( RUNE ); SetStyle( STYLE_RUNE );
	`						BEGIN( RAWSTRING ); SetStyle( STYLE_RAWSTRING );

	{WORD}					{
		if( kwGo.is(yytext) )
			SetStyle( STYLE_KEYWORD );
		else if( kwBuiltin.is(yytext) )
			SetStyle( STYLE_BUILTIN );
		else
			SetStyle( STYLE_TEXT );
	}

	 /* numbers - an "i" suffix makes an imaginary one */
	0[xX]_?{HEXS}?(\.{HEXS}?)?{HEXP}?i?	|
	0[bB]_?[01](_?[01])*i?				|
	0[oO]_?{OC}(_?{OC})*i?				|
	{DECS}(\.{DECS}?)?{DEXP}?i?			|
	\.{DECS}{DEXP}?i?					SetStyle( STYLE_NUMBER );

	 /* folding */
	[{(]					SetStyle( STYLE_SYMBOL ); levelIncrease();
	[})]					SetStyle( STYLE_SYMBOL ); levelDecrease();

	{SYMB}					SetStyle( STYLE_SYMBOL );
}

<COMMENT>{
	"*/"					SetStyle(); BEGIN( INITIAL ); levelDecrease();
	[^*\r\n]+				SetStyle();
}

<STRING>{
	\"						SetStyle(); BEGIN( INITIAL );
	[^"\\\r\n]+				SetStyle();
}

<RUNE>{
	'						SetStyle(); BEGIN( INITIAL );
	[^'\\\r\n]+				SetStyle();
}

<STRING,RUNE>{
	 /* escape sequences */
	\\[abfnrtv\\'"]			|
	\\{OC}{3}				|
	\\x{HC}{2}				|
	\\u{HC}{4}				|
	\\U{HC}{8}				SetStyle();

	 /* an invalid escape sequence */
	\\.						SetStyle( IStyler::styleInvalidText );

	 /* the string is not closed at the end of the line */
	{EOL}					SetStyle( STYLE_UNQ ); BEGIN( INITIAL );
}

 /* a raw string has no escapes and may span lines */
<RAWSTRING>{
	`						SetStyle(); BEGIN( INITIAL );
	[^`\r\n]+				SetStyle();
}

<*>{
	{WS}+	|
	{EOL}	|
	.						SetStyle();
}

%%

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case COMMENT:	return STYLE_COMMENT;
		case STRING:	return STYLE_STRING;
		case RUNE:		return STYLE_RUNE;
		case RAWSTRING:	return STYLE_RAWSTRING;
	}
	return STYLE_TEXT;
}
