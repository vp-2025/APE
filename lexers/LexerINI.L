%{
/******************************************************************************
* 		ini-like files: *.ini *.inf *.reg *.properties hgrc .gitconfig ...
*
*   ; comment  # comment            at the line start (and "  ; comment" after an ini value)
*   [section]  [remote "origin"]    section header, folds up to the next one
*   key = value   key: value        key, separator and value
*   "key"="value"  @=dword:0000001  registry values (m_langId == LANG_REG)
*   key value  ! comment            java properties (m_langId == LANG_PROPERTIES)
*   !include file                   MySQL style directive
*
* The whole value is styled as a number, a quoted string or a plain value.
* A '\' at the line end continues the value on the next line in properties
* and registry files.
******************************************************************************/
#include "LexBase.h"
%}

%option  c++
%option  prefix="ini_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x SECTION VALUE

EOL		(\r?\n)
WS		[ \t]
KEY		[^;#!=:\[\r\n \t]([^=:\r\n]*[^=:\r\n \t])?
QKEY	(\"([^"\\\r\n]|\\.)*\"|@)

%{
/* style ids - keep in sync with styleIni[] in lexBase/lexerStyles.cpp */
#define STYLE_TEXT			0
#define STYLE_COMMENT		1
#define STYLE_SECTION		2
#define STYLE_KEY			3
#define STYLE_SYMBOL		4
#define STYLE_VALUE			5
#define STYLE_STRING		6
#define STYLE_NUMBER		7
#define STYLE_DIRECTIVE		11

/* m_langId - the dialect, chosen by the file name in lexers.cpp */
#define LANG_INI			0
#define LANG_PROPERTIES		1
#define LANG_REG			2
%}

%%
 /*************************************************************************************/

%{
	bool bContinue = false;					// the line ends with '\': the value goes on at the next line
	bool bContLine = YY_START == VALUE;		// the line continues the value of the previous one

	auto isWS = []( char ch ) { return ch == ' ' || ch == '\t'; };

	auto beginValue = [&]() {
		BEGIN( VALUE );
	};

	// -12  3.5  1e-3  .5  0x1F
	auto isNumber = []( const char* p, int n ) {
		int i = 0, digits = 0;
		if( i < n && (p[i] == '-' || p[i] == '+') )
			i++;
		if( i+1 < n && p[i] == '0' && (p[i+1] == 'x' || p[i+1] == 'X') ) {
			for( i += 2; i < n && isxdigit((unsigned char)p[i]); i++ )
				digits++;
			return digits > 0 && i == n;
		}
		for( ; i < n && isdigit((unsigned char)p[i]); i++ )
			digits++;
		if( i < n && p[i] == '.' )
			for( i++; i < n && isdigit((unsigned char)p[i]); i++ )
				digits++;
		if( !digits )
			return false;
		if( i < n && (p[i] == 'e' || p[i] == 'E') ) {
			i++;
			if( i < n && (p[i] == '-' || p[i] == '+') )
				i++;
			int exp = i;
			while( i < n && isdigit((unsigned char)p[i]) )
				i++;
			if( i == exp )
				return false;
		}
		return i == n;
	};

	// position after the closing quote of the string at p[0], or -1: "a\"b"  'it'
	auto quotedEnd = []( const char* p, int n ) {
		char q = *p;
		if( n < 2 || (q != '"' && q != '\'') )
			return -1;
		for( int i = 1; i < n; i++ ) {
			if( q == '"' && p[i] == '\\' )
				i++;
			else if( p[i] == q )
				return i+1;
		}
		return -1;
	};

	// a properties key ends at the first space: "key value"
	auto styleKey = [&]() {
		if( m_langId == LANG_PROPERTIES ) {
			int k = 0;
			while( k < yyleng && isWS(yytext[k]) )
				k++;
			while( k < yyleng && !isWS(yytext[k]) )
				k++;
			if( k < yyleng ) {
				SetStyle( MatchStart(), MatchStart()+k, STYLE_KEY );
				yyless( k );
				beginValue();
				return;
			}
		}
		SetStyle( STYLE_KEY );
	};

%}

<INITIAL>{
	^(Windows\ Registry\ Editor\ Version\ [0-9.]+|REGEDIT4)	SetStyle( STYLE_DIRECTIVE );

	^{WS}*[;#][^\r\n]*		SetStyle( STYLE_COMMENT );
	^{WS}*![^\r\n]*			{
		if( m_langId == LANG_PROPERTIES )
			SetStyle( STYLE_COMMENT );
		else {
			int n = 0;
			while( isWS(yytext[n]) )
				n++;
			for( n++; n < yyleng && (isalnum((unsigned char)yytext[n]) || yytext[n] == '_'); n++ )
				;
			SetStyle( MatchStart(), MatchStart()+n, STYLE_DIRECTIVE );
			yyless( n );
			beginValue();
		}
	}

	^{WS}*\[				{
		SetStyle( STYLE_SYMBOL );
		BEGIN( SECTION );
		if( GetLevel() )
			levelDecrease();
		levelIncrease();
	}

	^{WS}*{QKEY}/{WS}*[=:]	SetStyle( STYLE_KEY );
	^{WS}*{KEY}/{WS}*[=:]	styleKey();

	 /* a line without a separator: a flag ("skip-networking"), a properties "key value"
	    or the indented continuation of the previous value (hgrc, python configparser) */
	^{WS}*{KEY}				{
		if( m_langId == LANG_INI && isWS(*yytext) ) {
			beginValue();
			yyless( 0 );
		} else
			styleKey();
	}

	{WS}*[=:]				SetStyle( STYLE_SYMBOL ); beginValue();

	{WS}+[;#][^\r\n]*		SetStyle( STYLE_COMMENT );		// after "[section]"
}

<SECTION>{
	\"([^"\\\r\n]|\\.)*\"	SetStyle( STYLE_STRING );
	[^\]"\r\n]+				SetStyle( STYLE_SECTION );
	\]						SetStyle( STYLE_SYMBOL ); BEGIN( INITIAL );
}

 /* the rest of the line after '=' gets a single style: a number, a quoted string
    or a plain value.  An ini value may be followed by a ; comment */
<VALUE>[^\r\n]+				{
	int b = 0, n = yyleng;
	while( b < n && isWS(yytext[b]) )
		b++;
	int e = n;							// value end
	if( m_langId == LANG_INI ) {
		int q = quotedEnd( yytext+b, n-b );
		for( int i = q > 0 ? b+q : b; i < n; i++ )
			if( yytext[i] == ';' && (i == b || isWS(yytext[i-1])) ) {
				e = i;
				break;
			}
	}
	int c = e;							// comment start
	while( e > b && isWS(yytext[e-1]) )
		e--;

	int style = STYLE_VALUE;
	if( m_langId != LANG_INI && e > b && yytext[e-1] == '\\' )
		bContinue = true;				// a multi-line value is a plain one
	else if( !bContLine ) {
		if( isNumber( yytext+b, e-b ) )
			style = STYLE_NUMBER;
		else if( quotedEnd( yytext+b, e-b ) == e-b )
			style = STYLE_STRING;
	}

	int pos = MatchStart();
	if( b )
		SetStyle( pos, pos+b, STYLE_VALUE );
	if( e > b )
		SetStyle( pos+b, pos+e, style );
	if( c > e )
		SetStyle( pos+e, pos+c, STYLE_VALUE );
	if( n > c )
		SetStyle( pos+c, pos+n, STYLE_COMMENT );
}

 /*************************************************************************************/
 /* default rule - a value, a section or a string ends with the line
  *************************************************************************************/

<*>{
	{EOL}					{
		SetStyle();
		if( !bContinue )
			BEGIN( INITIAL );
		bContLine = bContinue;
		bContinue = false;
	}
	{WS}+	|
	.						SetStyle();
}

%%
/*************************************************************************************/

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case SECTION:	return STYLE_SECTION;
		case VALUE:		return STYLE_VALUE;
	}
	return STYLE_TEXT;
}
