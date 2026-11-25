%{
/******************************************************************************
*		Python: *.py *.pyw *.pyi
*
*   keywords, builtins, the name after def / class, @decorators, numbers, # comments
*   strings with the prefixes r b u f (also rb, br, rf, fr): '...' "..." and the
*   triple-quoted '''...''' """...""" over several lines
*
*   folding by indentation: a code line indented deeper than the code line above
*   makes that one a fold header.  Blank and comment lines, lines inside brackets,
*   after a '\' and inside a triple-quoted string don't change the level.
*   The line state keeps the bracket depth and the '\' continuation (m_stateExt).
******************************************************************************/
#include "LexBase.h"
%}

%option  c++
%option  prefix="py_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x SQ DQ TSQ TDQ

SWC		[A-Za-z_\x80-\xFF]
WWC		[A-Za-z_0-9\x80-\xFF]
DC		[0-9]
HC		[0-9A-Fa-f]
EOL		(\r?\n)
WS		[ \t]
WORD	{SWC}{WWC}*
DECS	{DC}(_?{DC})*
DEXP	([eE][+-]?{DECS})
SPFX	([rRuUbBfF]|[rR][bBfF]|[bBfF][rR])
SYMB	[-+*/%&|^<>=!:.,;~@\\]

%{

#define STYLE_TEXT			0
#define STYLE_KEYWORD		1
#define STYLE_COMMENT		2
#define STYLE_SYMBOL		3
#define STYLE_STRING		4
#define STYLE_TSTRING		5
#define STYLE_NUMBER		6
#define STYLE_DECORATOR		7
#define STYLE_UNQ			8
#define STYLE_BUILTIN		9
#define STYLE_DEF			10

/* m_stateExt: the bracket depth in the low byte, CONT: the line ends with '\' */
#define DEPTH				(m_stateExt & 0xFF)
#define CONT				0x100

static CKeywords kwPython(
	"False None True and as assert async await break class continue def del elif else except "
	"finally for from global if import in is lambda nonlocal not or pass raise return try "
	"while with yield", true );
static CKeywords kwBuiltin(
	// functions and types
	"abs aiter all anext any ascii bin bool breakpoint bytearray bytes callable chr classmethod "
	"compile complex delattr dict dir divmod enumerate eval exec filter float format frozenset "
	"getattr globals hasattr hash help hex id input int isinstance issubclass iter len list "
	"locals map max memoryview min next object oct open ord pow print property range repr "
	"reversed round set setattr slice sorted staticmethod str sum super tuple type vars zip "
	"__import__ "
	// constants and the usual names
	"Ellipsis NotImplemented self cls __name__ __file__ __doc__ __main__ "
	// exceptions
	"BaseException Exception ArithmeticError AssertionError AttributeError BlockingIOError "
	"BrokenPipeError BufferError ConnectionError EOFError FileExistsError FileNotFoundError "
	"FloatingPointError GeneratorExit ImportError IndentationError IndexError InterruptedError "
	"IsADirectoryError KeyError KeyboardInterrupt LookupError MemoryError ModuleNotFoundError "
	"NameError NotADirectoryError NotImplementedError OSError OverflowError PermissionError "
	"RecursionError ReferenceError RuntimeError StopAsyncIteration StopIteration SyntaxError "
	"SystemError SystemExit TabError TimeoutError TypeError UnboundLocalError UnicodeDecodeError "
	"UnicodeEncodeError UnicodeError ValueError ZeroDivisionError IOError EnvironmentError "
	"Warning DeprecationWarning RuntimeWarning UserWarning", true );
%}

%%

%{
	int indent = 0;							// columns of the leading white space of the line
	bool bLineStart = YY_START == INITIAL && !(m_stateExt & CONT); // no token yet on a line, which starts a statement
	bool bDef = false;						// the next word is the name of a def / class
	bool bDot = false;						// the word follows a '.': an attribute, not a builtin

	// the last code line above and its fold level
	int prevLine = m_foldLine - 1;
	while( prevLine > 0 && (m_pStyler->GetLevel(prevLine) & SC_FOLDLEVELWHITEFLAG) )
		prevLine--;							// a blank, comment or continuation line in a block
	int prevLevel = prevLine >= 0 ? (m_pStyler->GetLevel(prevLine) & SC_FOLDLEVELNUMBERMASK) : SC_FOLDLEVELBASE;

	// the first token of a statement line: its indentation is its fold level
	auto codeToken = [&]() {
		bDot = false;
		if( !bLineStart )
			return;
		bLineStart = false;
		if( DEPTH > 0 )
			return;								// a continuation inside brackets
		int level = SC_FOLDLEVELBASE + indent;
		if( level > SC_FOLDLEVELNUMBERMASK )
			level = SC_FOLDLEVELNUMBERMASK;
		if( prevLine >= 0 && prevLine < m_foldLine ) {
			bool bHeader = level > prevLevel;
			m_pStyler->SetLevel( prevLine, prevLevel | (bHeader ? SC_FOLDLEVELHEADERFLAG : 0) );
			if( bHeader )						// the blank and comment lines after the header belong to the block
				for( int line = prevLine + 1; line < m_foldLine; line++ )
					m_pStyler->SetLevel( line, level | SC_FOLDLEVELWHITEFLAG );
		}
		m_foldLevel = level;
		prevLine = m_foldLine;
		prevLevel = level;
	};

	// the end of a line in INITIAL: the next one starts a statement
	auto endLine = [&]() {
		m_stateExt &= ~CONT;
		bLineStart = true;
		indent = 0;
		bDef = false;
	};
%}

<INITIAL>{
	^{WS}+					{
		indent = 0;
		for( int i = 0; i < yyleng; i++ )
			indent = yytext[i] == '\t' ? (indent / 8 + 1) * 8 : indent + 1;
		SetStyle( STYLE_TEXT );
	}

	"#"[^\r\n]*				SetStyle( STYLE_COMMENT );

	{SPFX}?\"\"\"			codeToken(); BEGIN( TDQ ); SetStyle( STYLE_TSTRING );
	{SPFX}?'''				codeToken(); BEGIN( TSQ ); SetStyle( STYLE_TSTRING );
	{SPFX}?\"				codeToken(); BEGIN( DQ ); SetStyle( STYLE_STRING );
	{SPFX}?'				codeToken(); BEGIN( SQ ); SetStyle( STYLE_STRING );

	 /* @decorator at the start of a statement, else the matrix multiplication */
	"@"{WS}*{WORD}(\.{WORD})*	{
		bool bDecorator = bLineStart;
		codeToken();
		if( bDecorator )
			SetStyle( STYLE_DECORATOR );
		else {
			yyless( 1 );
			SetStyle( STYLE_SYMBOL );
		}
	}

	{WORD}					{
		bool bAttr = bDot;
		codeToken();
		int style = STYLE_TEXT;
		if( bDef )
			style = STYLE_DEF, bDef = false;
		else if( kwPython.is(yytext) ) {
			style = STYLE_KEYWORD;
			bDef = !strcmp(yytext, "def") || !strcmp(yytext, "class");
		} else if( !bAttr && kwBuiltin.is(yytext) )
			style = STYLE_BUILTIN;
		SetStyle( style );
	}

	 /* numbers - "j" makes an imaginary one */
	0[xX](_?{HC})+			|
	0[oO](_?[0-7])+			|
	0[bB](_?[01])+			|
	{DECS}(\.{DECS}?)?{DEXP}?[jJ]?	|
	\.{DECS}{DEXP}?[jJ]?	codeToken(); SetStyle( STYLE_NUMBER );

	 /* brackets: the lines inside them continue the statement */
	[\[({]					codeToken(); SetStyle( STYLE_SYMBOL ); if( DEPTH < 0xFF ) m_stateExt++;
	[\])}]					codeToken(); SetStyle( STYLE_SYMBOL ); if( DEPTH > 0 ) m_stateExt--;

	 /* '\' at the end of the line: the next one continues the statement */
	\\{EOL}					{
		SetStyle( STYLE_SYMBOL );
		m_stateExt |= CONT;
		bLineStart = false;
		indent = 0;
		bDef = false;
	}

	{SYMB}					codeToken(); SetStyle( STYLE_SYMBOL ); bDot = *yytext == '.';

	{EOL}					SetStyle( CSciStyler::styleEOL ); endLine();
}

<SQ>{
	'						SetStyle(); BEGIN( INITIAL );
	[^'\\\r\n]+				SetStyle();
}

<DQ>{
	\"						SetStyle(); BEGIN( INITIAL );
	[^"\\\r\n]+				SetStyle();
}

<SQ,DQ>{
	\\{EOL}					SetStyle();				/* the string goes on at the next line */
	\\.						SetStyle();

	 /* the string is not closed at the end of the line */
	{EOL}					SetStyle( STYLE_UNQ ); BEGIN( INITIAL ); endLine();
}

<TSQ>{
	'''						SetStyle(); BEGIN( INITIAL );
	[^'\\\r\n]+				|
	'						SetStyle();
}

<TDQ>{
	\"\"\"					SetStyle(); BEGIN( INITIAL );
	[^"\\\r\n]+				|
	\"						SetStyle();
}

<TSQ,TDQ>{
	\\(.|{EOL})				SetStyle();
	{EOL}					SetStyle( CSciStyler::styleEOL );	/* the string goes on: the next line doesn't start a statement */
}

<*>{
	{WS}+	|
	.						SetStyle();
}

%%

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case SQ:
		case DQ:		return STYLE_STRING;
		case TSQ:
		case TDQ:		return STYLE_TSTRING;
	}
	return STYLE_TEXT;
}
