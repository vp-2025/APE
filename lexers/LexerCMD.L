%{
/******************************************************************************
*		Windows command scripts (cmd.exe): *.cmd
*
*   internal commands, keywords (if else for do in not exist defined equ ...),
*   external commands at the start of a command, /options, labels and the
*   labels of goto / call, rem and :: comments, echo text, strings,
*   variables: %name% %name:~0,5% %1 %~dp0 %* %%i %%~fi !delayed!, ^escapes
*
*   folding: ( ... ) blocks of if / for / else, and every :label up to the next one
*   The line state keeps the bracket depth and the '^' line continuation (m_stateExt).
******************************************************************************/
#include "LexBase.h"
%}

%option  c++
%option  prefix="cmd_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x COMMENT ECHO STRING

EOL		(\r?\n)
WS		[ \t]
CMDW	[^ \t\r\n&|<>()"^=,;%!@][^ \t\r\n&|<>()"^=,;%!]*
LABEL	[^ \t\r\n&|<>()"^:+=,;]+
REDIR	([0-9]?">"">"?(&[0-9])?|[0-9]?"<")

 /* %name%  %name:~0,5%  %name:a=b%  %1  %~dp0  %~$PATH:1  %*  %%i  %%~fi  !name! */
VARS	(%[A-Za-z_#$@][^%\r\n]*%|%(~[A-Za-z]*(\$[A-Za-z_]+:)?)?[0-9]|%\*|%%(~[A-Za-z]*(\$[A-Za-z_]+:)?)?[A-Za-z]|![A-Za-z_][^!\r\n]*!)

%{

#define STYLE_TEXT			0
#define STYLE_COMMAND		1
#define STYLE_EXTERNAL		2
#define STYLE_COMMENT		3
#define STYLE_ECHO			4
#define STYLE_STRING		5
#define STYLE_VAR			6
#define STYLE_LABEL			7
#define STYLE_SYMBOL		8
#define STYLE_UNQ			9
#define STYLE_KEYWORD		10
#define STYLE_OPTION		11
#define STYLE_LABELREF		12

/* m_stateExt: the bracket depth in the low byte, CONT: the line ends with '^' */
#define DEPTH				(m_stateExt & 0xFF)
#define CONT				0x100

static CKeywordsIC kwCommand(
	"assoc break call cd chdir cls color copy date del dir dpath echo endlocal erase exit "
	"for ftype goto if keys md mkdir mklink move path pause popd prompt pushd rd rem ren "
	"rename rmdir set setlocal shift start time title type ver verify vol", true );
static CKeywordsIC kwKeyword(
	"else do in not exist defined errorlevel cmdextversion equ neq lss leq gtr geq "
	"nul con prn aux enabledelayedexpansion disabledelayedexpansion enableextensions "
	"disableextensions", true );
%}

%%

%{
	bool bCmd = !(m_stateExt & CONT);		// a command can start here
	bool bArgs = false;						// "(" doesn't start a block of commands: for .. in (set), set /a x=(1+2)
	bool bLabelRef = false;					// the next word is a label: goto x
	bool bSetVar = false;					// the next word is the variable of set

	auto isWord = [&]( const char* sz ) { return !_stricmp(yytext, sz); };

	// a new command after & && | || or a new line
	auto newCommand = [&]() {
		bCmd = true;
		bArgs = bLabelRef = bSetVar = false;
	};

	auto endLine = [&]() {
		m_stateExt &= ~CONT;
		newCommand();
	};

	auto openBlock = [&]() {
		SetStyle( STYLE_SYMBOL );
		levelIncrease();
		if( DEPTH < 0xFF )
			m_stateExt++;
		bCmd = !bArgs;
		bLabelRef = bSetVar = false;
	};

	auto closeBlock = [&]() {
		SetStyle( STYLE_SYMBOL );
		if( DEPTH > 0 ) {
			levelDecrease();
			m_stateExt--;
		}
		bCmd = false;
	};
%}

<INITIAL>{
	 /* a label folds up to the next one */
	^{WS}*:{LABEL}			{
		SetStyle( STYLE_LABEL );
		if( GetLevel() > DEPTH )
			levelDecrease();
		levelIncrease();
		bCmd = false;
	}

	^{WS}*"::"[^\r\n]*		SetStyle( STYLE_COMMENT );
	^{WS}*@+				SetStyle( STYLE_SYMBOL ); bCmd = true;

	\"						BEGIN( STRING ); SetStyle( STYLE_STRING ); bCmd = bLabelRef = bSetVar = false;

	{VARS}					SetStyle( STYLE_VAR ); bCmd = false;

	 /* ^x is the char itself, '^' at the end of the line continues the command */
	\^{EOL}					SetStyle( STYLE_SYMBOL ); m_stateExt |= CONT;
	\^.						SetStyle( STYLE_SYMBOL );

	"&&"|"||"|"&"|"|"		SetStyle( STYLE_SYMBOL ); newCommand();
	{REDIR}					SetStyle( STYLE_SYMBOL );
	"("						openBlock();
	")"						closeBlock();

	 /* /options: /a /p /i /f /d /s /q /y /? /b:value */
	"/"[A-Za-z?][A-Za-z0-9:\-]*	SetStyle( STYLE_OPTION );

	{CMDW}					{
		// echo. echo: echo/ echo\ echo[ echo+ print an empty line or the text after it
		if( bCmd && yyleng > 4 && !_strnicmp(yytext, "echo", 4) && strchr(".:/\\[]+", yytext[4]) )
			yyless( 4 );

		if( bLabelRef || (bCmd && *yytext == ':') ) {	// goto label, goto :eof, call :label
			SetStyle( STYLE_LABELREF );
			bLabelRef = bCmd = false;
		} else
		if( bSetVar ) {									// set name=value, set /a name=...
			SetStyle( STYLE_VAR );
			bSetVar = false;
		} else
		if( kwCommand.is(yytext) ) {
			if( bCmd && isWord("rem") ) {
				BEGIN( COMMENT );
				SetStyle( STYLE_COMMENT );
			} else {
				SetStyle( STYLE_COMMAND );
				if( bCmd && isWord("echo") )
					BEGIN( ECHO );
			}
			bCmd = isWord("call");						// call script.cmd, call :label
			bLabelRef = isWord("goto");
			bSetVar = isWord("set");
			bArgs = bSetVar;
		} else
		if( kwKeyword.is(yytext) ) {
			SetStyle( STYLE_KEYWORD );
			bCmd = isWord("do") || isWord("else");
			if( isWord("in") )
				bArgs = true;
			else if( bCmd )
				bArgs = false;
		} else {
			SetStyle( bCmd ? STYLE_EXTERNAL : STYLE_TEXT );
			bCmd = false;
		}
	}

	"=="|[=,;]				SetStyle( STYLE_SYMBOL );

	{EOL}					SetStyle( CSciStyler::styleEOL ); endLine();
}

 /* rem: the rest of the line */
<COMMENT>{
	[^\r\n]+				SetStyle();
	{EOL}					SetStyle( CSciStyler::styleEOL ); BEGIN( INITIAL ); endLine();
}

 /* echo: the text up to & | > < and, inside a block, up to ')' */
<ECHO>{
	{VARS}					SetStyle( STYLE_VAR );
	\^{EOL}					SetStyle( STYLE_SYMBOL ); m_stateExt |= CONT;
	\^.						SetStyle( STYLE_SYMBOL );
	"&&"|"||"|"&"|"|"		SetStyle( STYLE_SYMBOL ); BEGIN( INITIAL ); newCommand();
	{REDIR}					SetStyle( STYLE_SYMBOL ); BEGIN( INITIAL ); bCmd = false;
	")"						{
		if( DEPTH > 0 ) {
			BEGIN( INITIAL );
			closeBlock();
		} else
			SetStyle();
	}
	[^%!^&|<>)\r\n0-9]+		|
	[%!0-9]					SetStyle();
	{EOL}					SetStyle( CSciStyler::styleEOL ); BEGIN( INITIAL ); endLine();
}

<STRING>{
	{VARS}					SetStyle( STYLE_VAR );
	\"						SetStyle(); BEGIN( INITIAL );
	[^"%!\r\n]+				|
	[%!]					SetStyle();
	 /* the string is not closed at the end of the line */
	{EOL}					SetStyle( STYLE_UNQ ); BEGIN( INITIAL ); endLine();
}

<*>{
	{WS}+	|
	.						SetStyle();
}

%%

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case COMMENT:	return STYLE_COMMENT;
		case ECHO:		return STYLE_ECHO;
		case STRING:	return STYLE_STRING;
	}
	return STYLE_TEXT;
}
