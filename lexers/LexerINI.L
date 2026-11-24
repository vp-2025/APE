%{
/******************************************************************************
* 		ini-like files: *.ini *.inf *.reg *.properties hgrc .gitconfig ...
*
*   ; comment  # comment            at the line start (and "  ; comment" after an ini value)
*   [section]  [remote "origin"]    section header, folds up to the next one
*   key = value   key: value        key, separator and value
*   "key"="value"  @=dword:0000001  registry values (m_langId == LANG_REG)
*   key value  ! comment            java properties (m_langId == LANG_PROPERTIES)
*   mea.ini with mea-dg.ini         an ini with a meta file (m_langId == LANG_DG, see lexIniDg.h):
*                                   unknown sections and keys unknown in their section are marked
*   !include file                   MySQL style directive
*
* The whole value is styled as a number, a quoted string or a plain value.
* A '\' at the line end continues the value on the next line in properties
* and registry files.
*
* Sections alternate their background: every token style exists twice, the
* plain one for even sections (and the lines before the first section) and a
* tinted copy for odd sections (see styleIni[] in lexBase/lexerStyles.cpp).
******************************************************************************/
#include "LexBase.h"
#include "lexIniDg.h"
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
#define STYLE_TEXT			1
#define STYLE_COMMENT		2
#define STYLE_SECTION		3
#define STYLE_KEY			4
#define STYLE_SYMBOL		5
#define STYLE_VALUE			6
#define STYLE_STRING		7
#define STYLE_NUMBER		8
#define STYLE_DIRECTIVE		9
#define STYLE_UNKNOWN_KEY	10		/* LANG_DG: not in the meta file for this section */
#define STYLE_UNKNOWN_SEC	11		/* LANG_DG: not in the meta file */

/* even sections use the ids 2..12, odd ones are shifted by STYLE_ODD - the ids 42..52 (jump over STYLE_DEFAULT 32..39) */
#define STYLE_EVEN			1
#define STYLE_ODD			41
#define SEC( styl )			( bOddSection ? STYLE_ODD + (styl) : STYLE_EVEN + (styl) )

/* m_langId - the dialect, chosen by the file name in lexers.cpp */
#define LANG_INI			0
#define LANG_PROPERTIES		1
#define LANG_REG			2
#define LANG_DG				3		/* ini with a meta file: g_pIniDgLexing */

/* LANG_DG: the section of the line in m_stateExt: 0 before the first one, else its index+1 in the meta file */
#define SECTION_UNKNOWN		0xFFF

/* yy_start keeps the start condition in its low 8 bits only.  The parity of
   the current section is kept above them and serialized into the line state
   at every newline, so that styling can resume in the middle of the file. */
#define LINE_STATE			((yy_start & 0xFF) | (bOddSection ? 0x100 : 0) | (m_stateExt << 16))

/* StateToStyle() is called by the rules - declare the specialization first */
template<> int CSciLexer<yyFlexLexer>::StateToStyle(int nState);

#undef	YY_BREAK
#define YY_BREAK \
	if( yyleng > 0 && yytext[yyleng-1] == '\n' ) \
		m_pStyler->SetLineState( m_foldLine, LINE_STATE ); \
	break;
%}

%%

%{
	bool bOddSection = (yy_start & 0x100) != 0;	// background of the current section
	yy_start &= 0xFF;						// the DFA only understands the start condition

	bool bContinue = false;					// the line ends with '\': the value goes on at the next line
	bool bContLine = YY_START == VALUE;		// the line continues the value of the previous one

	auto isWS = []( char ch ) { return ch == ' ' || ch == '\t'; };

	const bool bIni = m_langId == LANG_INI || m_langId == LANG_DG;	// ; comments after a value, indented continuations
	const CIniDgSchema* pSchema = m_langId == LANG_DG ? g_pIniDgLexing : nullptr;

	// LANG_DG: the key is known in its section; an unknown section is marked itself
	auto isKnownKey = [&]( const char* p, int n ) {
		if( !pSchema || m_stateExt == SECTION_UNKNOWN )
			return true;
		if( m_stateExt <= 0 || m_stateExt > (int)pSchema->sections.size() )
			return false;							// before the first section
		return pSchema->sections[m_stateExt-1].Find( std::string(p, n) ) != nullptr;
	};

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
				SetStyle( MatchStart(), MatchStart()+k, SEC(STYLE_KEY) );
				yyless( k );
				beginValue();
				return;
			}
		}
		int b = 0;
		while( b < yyleng && isWS(yytext[b]) )
			b++;
		if( b )
			SetStyle( MatchStart(), MatchStart()+b, SEC(STYLE_TEXT) );
		SetStyle( MatchStart()+b, MatchEnd(), SEC(isKnownKey(yytext+b, yyleng-b) ? STYLE_KEY : STYLE_UNKNOWN_KEY) );
	};

%}

<INITIAL>{
	^(Windows\ Registry\ Editor\ Version\ [0-9.]+|REGEDIT4)	SetStyle( SEC(STYLE_DIRECTIVE) );

	^{WS}*[;#][^\r\n]*		SetStyle( SEC(STYLE_COMMENT) );
	^{WS}*![^\r\n]*			{
		if( m_langId == LANG_PROPERTIES )
			SetStyle( SEC(STYLE_COMMENT) );
		else {
			int n = 0;
			while( isWS(yytext[n]) )
				n++;
			for( n++; n < yyleng && (isalnum((unsigned char)yytext[n]) || yytext[n] == '_'); n++ )
				;
			SetStyle( MatchStart(), MatchStart()+n, SEC(STYLE_DIRECTIVE) );
			yyless( n );
			beginValue();
		}
	}

	^{WS}*\[				{
		bOddSection = !bOddSection;
		if( pSchema )
			m_stateExt = SECTION_UNKNOWN;	// till its name is known
		SetStyle( SEC(STYLE_SYMBOL) );
		BEGIN( SECTION );
		if( GetLevel() )
			levelDecrease();
		levelIncrease();
	}

	^{WS}*{QKEY}/{WS}*[=:]	SetStyle( SEC(STYLE_KEY) );
	^{WS}*{KEY}/{WS}*[=:]	styleKey();

	 /* a line without a separator: a flag ("skip-networking"), a properties "key value"
	    or the indented continuation of the previous value (hgrc, python configparser) */
	^{WS}*{KEY}				{
		if( bIni && isWS(*yytext) ) {
			beginValue();
			yyless( 0 );
		} else
			styleKey();
	}

	{WS}*[=:]				SetStyle( SEC(STYLE_SYMBOL) ); beginValue();

	{WS}+[;#][^\r\n]*		SetStyle( SEC(STYLE_COMMENT) );		// after "[section]"
}

<SECTION>{
	\"([^"\\\r\n]|\\.)*\"	SetStyle( SEC(STYLE_STRING) );
	[^\]"\r\n]+				{
		int style = STYLE_SECTION;
		if( pSchema ) {
			int i = pSchema->FindSection( std::string(yytext, yyleng) );
			m_stateExt = i >= 0 ? i+1 : SECTION_UNKNOWN;
			if( i < 0 )
				style = STYLE_UNKNOWN_SEC;
		}
		SetStyle( SEC(style) );
	}
	\]						SetStyle( SEC(STYLE_SYMBOL) ); BEGIN( INITIAL );
}

 /* the rest of the line after '=' gets a single style: a number, a quoted string
    or a plain value.  An ini value may be followed by a ; comment */
<VALUE>[^\r\n]+				{
	int b = 0, n = yyleng;
	while( b < n && isWS(yytext[b]) )
		b++;
	int e = n;							// value end
	if( bIni ) {
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
	if( !bIni && e > b && yytext[e-1] == '\\' )
		bContinue = true;				// a multi-line value is a plain one
	else if( !bContLine ) {
		if( isNumber( yytext+b, e-b ) )
			style = STYLE_NUMBER;
		else if( quotedEnd( yytext+b, e-b ) == e-b )
			style = STYLE_STRING;
	}

	int pos = MatchStart();
	if( b )
		SetStyle( pos, pos+b, SEC(STYLE_VALUE) );
	if( e > b )
		SetStyle( pos+b, pos+e, SEC(style) );
	if( c > e )
		SetStyle( pos+e, pos+c, SEC(STYLE_VALUE) );
	if( n > c )
		SetStyle( pos+c, pos+n, SEC(STYLE_COMMENT) );
}

 /*************************************************************************************/
 /* default rule - a value, a section or a string ends with the line
  *************************************************************************************/

<*>{
	{EOL}					{
		SetStyle( SEC(StateToStyle(YY_START)) );
		if( !bContinue )
			BEGIN( INITIAL );
		bContLine = bContinue;
		bContinue = false;
	}
	{WS}+	|
	.						SetStyle( SEC(StateToStyle(YY_START)) );
}

%%

template<> int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case SECTION:	return STYLE_SECTION;
		case VALUE:		return STYLE_VALUE;
	}
	return STYLE_TEXT;
}
