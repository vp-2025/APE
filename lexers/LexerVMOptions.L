%{
/******************************************************************************
* 		*.vmoptions - JVM options of a Java launcher (JetBrains IDEs, install4j)
*
*   # comment                       a line starting with '#'
*   -Xmx2048m -Xss2m                -X options, the size follows the name
*   -XX:+UseG1GC -XX:-Flag          boolean -XX option, '+' enables / '-' disables
*   -XX:ReservedCodeCacheSize=512m  -XX option with a value
*   -Dfile.encoding=UTF-8           system property
*   --add-opens=java.base/java.lang=ALL-UNNAMED
*   -javaagent:/path/agent.jar      ...any other option, its value after ':' / '='
*   ${VAR}                          variable (install4j)
*
* Every line holds one option, so an option is recognized only as the first
* token of a line.
******************************************************************************/
#include "LexBase.h"
%}

%option  c++
%option  prefix="vmoptions_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x VALUE

EOL		(\r?\n)
WS		[ \t]
NAME	[A-Za-z_][A-Za-z0-9_.]*
OPTION	--?[A-Za-z][A-Za-z_.\-]*
NUM		[0-9]+(\.[0-9]+)?[kKmMgGtT%]?
TOKEN	[^ \t\r\n=:,$]+

%{
/* style ids - keep in sync with styleVMOptions[] in lexBase/lexerStyles.cpp */
#define STYLE_TEXT			0
#define STYLE_COMMENT		1
#define STYLE_OPTION		2
#define STYLE_NAME			3
#define STYLE_ENABLED		4
#define STYLE_DISABLED		5
#define STYLE_VALUE			6
#define STYLE_NUMBER		7
#define STYLE_SYMBOL		8
#define STYLE_VARIABLE		9
%}

%%

 /* the first token of a line: a comment, an option or plain text */
<INITIAL>{
	^{WS}*#[^\r\n]*			SetStyle( STYLE_COMMENT );

	 /* -XX:+Flag -XX:-Flag -XX:Name=value */
	"-XX:"[+-]?{NAME}		{
		int n = 4;
		SetStyle( MatchStart(), MatchStart()+n, STYLE_OPTION );
		if( yytext[n]=='+' || yytext[n]=='-' ) {
			SetStyle( MatchStart()+n, MatchStart()+n+1, yytext[n]=='+' ? STYLE_ENABLED : STYLE_DISABLED );
			n++;
		}
		SetStyle( MatchStart()+n, MatchEnd(), STYLE_NAME );
		BEGIN(VALUE);
	}

	 /* -Dname=value */
	"-D"[^ \t\r\n=]+		{
		SetStyle( MatchStart(), MatchStart()+2, STYLE_OPTION );
		SetStyle( MatchStart()+2, MatchEnd(), STYLE_NAME );
		BEGIN(VALUE);
	}

	 /* no digits in the name: -Xmx2048m is the option -Xmx with the value 2048m */
	{OPTION}				SetStyle( STYLE_OPTION ); BEGIN(VALUE);

	[^\-# \t\r\n][^\r\n]*	SetStyle( STYLE_TEXT );
	{WS}+	|
	.						SetStyle( STYLE_TEXT );
}

 /* the rest of the line: the value of the option */
<VALUE>{
	{NUM}					SetStyle( STYLE_NUMBER );
	{TOKEN}					SetStyle( STYLE_VALUE );
	"${"[^}\r\n]*"}"		SetStyle( STYLE_VARIABLE );
	[=:,]					SetStyle( STYLE_SYMBOL );
}

<*>{
	{EOL}					BEGIN(INITIAL); SetStyle( IStyler::styleEOL );
	{WS}+	|
	.						SetStyle();
}

%%

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case VALUE:		return STYLE_VALUE;
	}
	return STYLE_TEXT;
}
