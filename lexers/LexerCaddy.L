%{
/******************************************************************************
* 		Caddyfile - the Caddy web server config
*
*   # comment                       a '#' starting a token, up to the line end
*   { ... }                         global options block (first block in the file)
*   example.com, :8080 { ... }      site block - addresses at the top level
*   (snippet) { ... }               snippet, used by "import snippet"
*   &(route) { ... }                named route
*   directive [matcher] args {      the first token of a line inside a block
*   @name type args                 named matcher definition
*   "str" `str` <<EOF ... EOF       quoted tokens and heredocs
*   {placeholder} {$ENV} {env.ENV}  placeholders, may appear inside any token
*
* Tokens are separated by whitespace: '{', '}', '#' and quotes are special only
* at the start of a token ("foo{x}" is a word with a placeholder, "a#b" is a word).
******************************************************************************/
#include "LexBase.h"
%}

%option  c++
%option  prefix="caddy_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x STRING BACKTICK HEREDOC

EOL		(\r?\n)
WS		[ \t]
WORD	[^ \t\r\n{}"`][^ \t\r\n{}]*
NUM		[-+]?[0-9]+(\.[0-9]+)?(ns|us|ms|s|m|h|d|[kKMGTP]i?[bB]?|[bB])?
PH		\{[^ \t\r\n{}"`]+\}

%{
/* style ids - keep in sync with styleCaddy[] in lexBase/lexerStyles.cpp */
#define STYLE_TEXT			0
#define STYLE_COMMENT		1
#define STYLE_SITE			2
#define STYLE_DIRECTIVE		3
#define STYLE_SUBDIR		4
#define STYLE_MATCHER		5
#define STYLE_MATCHERTYPE	6
#define STYLE_SNIPPET		7
#define STYLE_STRING		8
#define STYLE_BACKTICK		9
#define STYLE_HEREDOC		10
#define STYLE_PLACEHOLDER	11
#define STYLE_ENV			12
#define STYLE_NUMBER		13
#define STYLE_SYMBOL		14

/* yy_start keeps the start condition in its low 8 bits only.  The hash of the
   heredoc marker is kept in the upper bits and serialized into the line state
   at every newline, so that styling can resume inside a heredoc. */
#define LINE_STATE \
	((yy_start & 0xFF) | ((heredocHash & 0x7FFFFF) << 8))

#undef	YY_BREAK
#define YY_BREAK \
	if( yyleng > 0 && yytext[yyleng-1] == '\n' ) \
		m_pStyler->SetLineState( m_foldLine, LINE_STATE ); \
	break;
%}

%%
 /*************************************************************************************/

%{
	int extra = yy_start >> 8;
	yy_start &= 0xFF;						// the DFA only understands the start condition
	int heredocHash = extra & 0x7FFFFF;		// marker closing the current heredoc

	int  tokCount = 0;			// tokens started on the current line
	bool prevWS = true;			// a new token starts here (line start or after a space)
	bool matcherDef = false;	// the line starts with "@name": the next token is a matcher type
	bool importLine = false;	// the line starts with "import": the next token is a snippet

	static CKeywords kwDirective(
		// directives
		"abort acme_server basic_auth basicauth bind copy_response copy_response_headers "
		"encode error file_server forward_auth fs handle handle_errors handle_path header "
		"import intercept invoke log log_append log_name log_skip map method metrics "
		"php_fastcgi push redir request_body request_header respond reverse_proxy rewrite "
		"root route skip_log templates tls tracing try_files uri vars "
		// global options
		"debug http_port https_port default_bind order storage storage_check "
		"storage_clean_interval renew_interval ocsp_interval admin persist_config "
		"grace_period shutdown_delay auto_https email default_sni fallback_sni local_certs "
		"skip_install_trust acme_ca acme_ca_root acme_eab acme_dns dns ech on_demand_tls "
		"key_type cert_issuer ocsp_stapling preferred_chains servers pki events filesystem "
		"cert_lifetime renewal_window_ratio", true );
	static CKeywords kwMatcher(
		"client_ip expression file header header_regexp host method not path path_regexp "
		"protocol query remote_ip vars vars_regexp", true );

	auto hashOf = []( const char* p, int n ) {
		unsigned h = 0;
		for( int i = 0; i < n; i++ )
			h = h*31 + (unsigned char)p[i];
		return (int)( ((h ^ (h >> 16)) & 0xFFFF) | ((n & 0x7F) << 16) );
	};

	// returns true when the match starts a new token
	auto newToken = [&]() {
		bool b = prevWS;
		if( b )
			tokCount++;
		prevWS = false;
		return b;
	};

	auto styleWord = [&]( bool bStart ) {
		int idx = tokCount-1;
		int style = STYLE_TEXT;
		if( bStart && idx == 0 && !strcmp(yytext, "import") )
			importLine = true;
		if( bStart && idx == 1 && importLine )
			style = STYLE_SNIPPET;
		else if( GetLevel() <= 0 )
			style = bStart && idx == 0 && kwDirective.is(yytext) ? STYLE_DIRECTIVE : STYLE_SITE;
		else if( !bStart )
			style = STYLE_TEXT;
		else if( idx == 0 )
			style = kwDirective.is(yytext) ? STYLE_DIRECTIVE : STYLE_SUBDIR;
		else if( idx == 1 && matcherDef && kwMatcher.is(yytext) )
			style = STYLE_MATCHERTYPE;
		else if( idx == 1 && (*yytext == '/' || (*yytext == '*' && yyleng == 1)) )
			style = STYLE_MATCHER;
		SetStyle( style );
	};

	auto stylePlaceholder = [&]() {
		bool bEnv = yytext[1] == '$' || (yyleng > 5 && string(yytext, 5) == "{env.");
		SetStyle( bEnv ? STYLE_ENV : STYLE_PLACEHOLDER );
	};
%}

<INITIAL>{
	#[^\r\n]*				{
		if( prevWS )
			SetStyle( STYLE_COMMENT );
		else {
			yyless(1);
			SetStyle( STYLE_TEXT );
		}
	}

	"<<"[A-Za-z0-9_\-]+/{WS}*{EOL}	{
		if( newToken() ) {
			heredocHash = hashOf( yytext+2, yyleng-2 );
			BEGIN( HEREDOC );
			SetStyle( STYLE_SYMBOL );
		} else
			styleWord( false );
	}

	\{/[ \t\r\n]			{
		if( newToken() ) {
			SetStyle( STYLE_SYMBOL );
			levelIncrease();
		} else
			SetStyle( STYLE_TEXT );
	}
	\}						{
		if( newToken() ) {
			SetStyle( STYLE_SYMBOL );
			levelDecrease();
		} else
			SetStyle( STYLE_TEXT );
	}

	{PH}					newToken(); stylePlaceholder();

	\"						{
		if( newToken() ) {
			BEGIN( STRING );
			SetStyle( STYLE_STRING );
		} else
			SetStyle( STYLE_TEXT );
	}
	`						{
		if( newToken() ) {
			BEGIN( BACKTICK );
			SetStyle( STYLE_BACKTICK );
		} else
			SetStyle( STYLE_TEXT );
	}

	@[^ \t\r\n{}]+			{
		if( newToken() ) {
			if( tokCount == 1 )
				matcherDef = true;
			SetStyle( STYLE_MATCHER );
		} else
			styleWord( false );
	}

	&?\([^ \t\r\n()]+\)		{
		bool b = newToken();
		if( b && tokCount == 1 && GetLevel() <= 0 )
			SetStyle( STYLE_SNIPPET );
		else
			styleWord( b );
	}

	{NUM}/[ \t\r\n]			{
		bool b = newToken();
		if( b && tokCount > 1 && GetLevel() > 0 && !importLine )
			SetStyle( STYLE_NUMBER );
		else
			styleWord( b );
	}

	{WORD}					styleWord( newToken() );
}

<STRING>{
	\\.						SetStyle();
	{PH}					stylePlaceholder();
	\"						SetStyle(); BEGIN( INITIAL );
	[^"\\{\r\n]+			SetStyle();
}

<BACKTICK>{
	{PH}					stylePlaceholder();
	`						SetStyle(); BEGIN( INITIAL );
	[^`{\r\n]+				SetStyle();
}

 /* the heredoc ends at a line starting with its marker ("HTML 200"), the indent is allowed.
    The match always spans a whole line, the rest after the marker is rescanned as tokens. */
<HEREDOC>[^\r\n]+			{
	int b = 0, e;
	while( b < yyleng && (yytext[b] == ' ' || yytext[b] == '\t') )
		b++;
	for( e = b; e < yyleng && (isalnum((unsigned char)yytext[e]) || yytext[e] == '_' || yytext[e] == '-'); e++ )
		;
	if( e > b && (e == yyleng || yytext[e] == ' ' || yytext[e] == '\t') && hashOf( yytext+b, e-b ) == heredocHash ) {
		if( b )
			SetStyle( MatchStart(), MatchStart()+b, STYLE_HEREDOC );
		SetStyle( MatchStart()+b, MatchStart()+e, STYLE_SYMBOL );
		yyless( e );
		heredocHash = 0;
		tokCount = 1;
		prevWS = false;
		BEGIN( INITIAL );
	} else
		SetStyle( STYLE_HEREDOC );
}

 /*************************************************************************************/
 /* default rule
  *************************************************************************************/

<*>{
	{EOL}		{
		tokCount = 0;
		prevWS = true;
		matcherDef = importLine = false;
		SetStyle();
	}
	{WS}+		prevWS = true; SetStyle();
	.			prevWS = false; SetStyle();
}

%%
 /*************************************************************************************/

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case STRING:	return STYLE_STRING;
		case BACKTICK:	return STYLE_BACKTICK;
		case HEREDOC:	return STYLE_HEREDOC;
	}
	return STYLE_TEXT;
}
