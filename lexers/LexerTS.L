%{
/******************************************************************************
*		TypeScript: *.ts *.mts *.cts
*
*   keywords of JavaScript and TypeScript, the built-in types and global objects,
*   @decorators, numbers (0x 0o 0b, 1_000, 10n), 'strings' "strings",
*   `template ${literals}` - also nested, /regular expressions/flags,
*   // comments, /* comments * /, /** doc comments * /
*
*   a '/' starts a regular expression where a value is expected: at the start,
*   after an operator, ( , [ , { or a keyword like return - else it divides.
*   folding: { } ( ) [ ] and the multi-line comments
*
*   m_stateExt (saved in the line state): the open ${ } of templates, a stack of
*   3 levels - the count in bits 0..1, the brace depth inside each level in 4 bits
******************************************************************************/
#include "LexBase.h"
%}

%option  c++
%option  prefix="ts_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x COMMENT DOCCOMMENT SQ DQ TEMPLATE

SWC		[A-Za-z_$\x80-\xFF]
WWC		[A-Za-z_$0-9\x80-\xFF]
DC		[0-9]
HC		[0-9A-Fa-f]
EOL		(\r?\n)
WS		[ \t]
WORD	{SWC}{WWC}*
DECS	{DC}(_?{DC})*
HEXS	{HC}(_?{HC})*
DEXP	([eE][+-]?{DECS})
SYMB	[-+*/%&|^~<>=!?:;,.]

 /* the first char of a regular expression is not * or / (a comment) */
RECLASS	\[(\\.|[^\]\\\r\n])*\]
REGEXP	\/([^*\/\\\[\r\n]|\\.|{RECLASS})([^\/\\\[\r\n]|\\.|{RECLASS})*\/[dgimsuyv]*

%{

#define STYLE_TEXT			0
#define STYLE_KEYWORD		1
#define STYLE_COMMENT		2
#define STYLE_SYMBOL		3
#define STYLE_STRING		4
#define STYLE_TEMPLATE		5
#define STYLE_NUMBER		6
#define STYLE_DECORATOR		7
#define STYLE_UNQ			8
#define STYLE_TYPE			9
#define STYLE_REGEXP		10
#define STYLE_DOCCOMMENT	11
#define STYLE_SUBST			12

static CKeywords kwTS(
	// JavaScript
	"async await break case catch class const continue debugger default delete do else "
	"export extends false finally for function if import in instanceof let new null of "
	"return static super switch this throw true try typeof var void while with yield "
	// TypeScript
	"abstract accessor as asserts declare enum get implements infer interface is keyof "
	"module namespace out override package private protected public readonly require "
	"satisfies set type unique", true );
static CKeywords kwType(
	// the built-in types
	"any bigint boolean never number object string symbol undefined unknown void "
	// the utility types
	"Awaited Capitalize ConstructorParameters Exclude Extract InstanceType Lowercase "
	"NoInfer NonNullable Omit OmitThisParameter Parameters Partial Pick Readonly Record "
	"Required ReturnType ThisParameterType ThisType Uncapitalize Uppercase "
	// the global objects
	"Array ArrayBuffer BigInt Boolean DataView Date Error EvalError Function Generator "
	"Infinity Intl Iterable Iterator JSON Map Math NaN Number Object Promise PromiseLike "
	"Proxy RangeError ReadonlyArray ReadonlyMap ReadonlySet ReferenceError Reflect RegExp "
	"Set SharedArrayBuffer String Symbol SyntaxError TypeError URIError WeakMap WeakRef "
	"WeakSet Float32Array Float64Array Int8Array Int16Array Int32Array Uint8Array "
	"Uint8ClampedArray Uint16Array Uint32Array BigInt64Array BigUint64Array "
	"console document globalThis window process module exports", true );

/* the regular expression is allowed after these keywords - else after a word it divides */
static CKeywords kwBeforeValue(
	"return typeof instanceof in of new delete void throw case do else yield await", true );

/* m_stateExt: the stack of the open ${ } */
#define SUBST_COUNT			(m_stateExt & 3)
#define SUBST_SHIFT(i)		(2 + 4*(i))
#define SUBST_DEPTH(i)		((m_stateExt >> SUBST_SHIFT(i)) & 0xF)
#define SUBST_MAX			3
%}

%%

%{
	bool bValue = true;						// a value is expected: a '/' starts a regular expression

	auto setDepth = [&]( int i, int depth ) {
		m_stateExt = (m_stateExt & ~(0xF << SUBST_SHIFT(i))) | ((depth & 0xF) << SUBST_SHIFT(i));
	};
%}

<INITIAL>{
	^"#!"[^\r\n]*			SetStyle( STYLE_COMMENT );			/* #!/usr/bin/env node */

	"//"[^\r\n]*			SetStyle( STYLE_COMMENT );
	"/**"/[^/]				BEGIN( DOCCOMMENT ); SetStyle( STYLE_DOCCOMMENT ); levelIncrease();
	"/*"					BEGIN( COMMENT ); SetStyle( STYLE_COMMENT ); levelIncrease();

	{REGEXP}				{
		if( bValue ) {
			SetStyle( STYLE_REGEXP );
			bValue = false;
		} else {
			yyless( 1 );
			SetStyle( STYLE_SYMBOL );
			bValue = true;
		}
	}

	\'						BEGIN( SQ ); SetStyle( STYLE_STRING );
	\"						BEGIN( DQ ); SetStyle( STYLE_STRING );
	`						BEGIN( TEMPLATE ); SetStyle( STYLE_TEMPLATE );

	"@"{WORD}(\.{WORD})*	SetStyle( STYLE_DECORATOR ); bValue = false;

	"#"?{WORD}				{
		if( kwTS.is(yytext) ) {
			SetStyle( STYLE_KEYWORD );
			bValue = kwBeforeValue.is(yytext);
		} else {
			SetStyle( kwType.is(yytext) ? STYLE_TYPE : STYLE_TEXT );
			bValue = false;
		}
	}

	 /* numbers - "n" makes a BigInt */
	0[xX]{HEXS}n?			|
	0[oO][0-7](_?[0-7])*n?	|
	0[bB][01](_?[01])*n?	|
	{DECS}n					|
	{DECS}(\.{DECS}?)?{DEXP}?	|
	\.{DECS}{DEXP}?			SetStyle( STYLE_NUMBER ); bValue = false;

	"{"						{
		SetStyle( STYLE_SYMBOL );
		levelIncrease();
		if( SUBST_COUNT > 0 )
			setDepth( SUBST_COUNT-1, SUBST_DEPTH(SUBST_COUNT-1) + 1 );
		bValue = true;
	}
	"}"						{
		levelDecrease();
		if( SUBST_COUNT > 0 && SUBST_DEPTH(SUBST_COUNT-1) == 0 ) {	// the end of ${ }: the template goes on
			m_stateExt--;
			BEGIN( TEMPLATE );
			SetStyle( STYLE_SUBST );
		} else {
			if( SUBST_COUNT > 0 )
				setDepth( SUBST_COUNT-1, SUBST_DEPTH(SUBST_COUNT-1) - 1 );
			SetStyle( STYLE_SYMBOL );
		}
		bValue = false;
	}
	[(\[]					SetStyle( STYLE_SYMBOL ); levelIncrease(); bValue = true;
	[)\]]					SetStyle( STYLE_SYMBOL ); levelDecrease(); bValue = false;

	"++"|"--"				SetStyle( STYLE_SYMBOL );			/* i++ / x - doesn't change, what is expected */
	{SYMB}					SetStyle( STYLE_SYMBOL ); bValue = true;

	{EOL}					SetStyle( CSciStyler::styleEOL );
}

<COMMENT>{
	"*/"					SetStyle(); BEGIN( INITIAL ); levelDecrease();
	[^*\r\n]+				|
	"*"						SetStyle();
}

<DOCCOMMENT>{
	"*/"					SetStyle(); BEGIN( INITIAL ); levelDecrease();
	[^*\r\n]+				|
	"*"						SetStyle();
}

<SQ>{
	\'						SetStyle(); BEGIN( INITIAL ); bValue = false;
	[^'\\\r\n]+				SetStyle();
}

<DQ>{
	\"						SetStyle(); BEGIN( INITIAL ); bValue = false;
	[^"\\\r\n]+				SetStyle();
}

<SQ,DQ>{
	\\{EOL}					SetStyle();				/* the string goes on at the next line */
	\\.						SetStyle();
	 /* the string is not closed at the end of the line */
	{EOL}					SetStyle( STYLE_UNQ ); BEGIN( INITIAL ); bValue = false;
}

<TEMPLATE>{
	`						SetStyle(); BEGIN( INITIAL ); bValue = false;
	"${"					{
		if( SUBST_COUNT < SUBST_MAX ) {
			setDepth( SUBST_COUNT, 0 );
			m_stateExt++;
			BEGIN( INITIAL );
			SetStyle( STYLE_SUBST );
			levelIncrease();
			bValue = true;
		} else
			SetStyle();				// too deep: stays text of the template
	}
	\\(.|{EOL})				SetStyle();
	[^`$\\\r\n]+			|
	"$"						SetStyle();
	{EOL}					SetStyle( CSciStyler::styleEOL );
}

<*>{
	{EOL}					SetStyle( CSciStyler::styleEOL );
	{WS}+	|
	.						SetStyle();
}

%%

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case COMMENT:		return STYLE_COMMENT;
		case DOCCOMMENT:	return STYLE_DOCCOMMENT;
		case SQ:
		case DQ:			return STYLE_STRING;
		case TEMPLATE:		return STYLE_TEMPLATE;
	}
	return STYLE_TEXT;
}
