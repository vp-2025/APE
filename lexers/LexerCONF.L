%{
#include "LexBase.h"
%}

%option  c++
%option  prefix="conf_"
%option  yyclass="CSciLexer<yyFlexLexer>"

%x SLCOMMENT STRING TAG TAG_STRING

SWC		[A-Za-z_]
WWC		[_A-Za-z0-9]
OC		[0-7]
DC		[0-9]
HC		[0-9A-Fa-f]
EOL		(\r?\n)
WS		[ \t]
SPACE	{WS}*
WORD	{SWC}{WWC}*
SYMB	[<>(){}\[\]+\-*=&|~`\/\\,.:;]
	// without - "'	esc []-\/

%{

bool isApacheKeyword( const char* szKW ) {
	static set<string> stKW = {"ProxyPass", "ProxyRequests", "AddDefaultCharset",
		"AuthType", "AuthName", "AuthBasicProvider","AuthUserfile", "AuthnzFcgiDefineProvider",
		"Alias", "Require", "AllowOverride", "RequestHeader", "Options", "AddHandler", "DirectoryIndex",
		"ServerRoot", "ServerAdmin", "ServerName", "DocumentRoot", "Listen", "LoadModule"};
	return stKW.find(szKW)!=stKW.end();
}

bool isApacheKeyword2( const char* szKW ) {
	static set<string> stKW = {"none", "all", "granted", "denied", "set", "Basic", "ExecCGI", "Indexes",
		"authn", "authz", "authnz"};
	return stKW.find(szKW)!=stKW.end();
}

bool isNginxKeyword( const char* szKW ) {
	static set<string> stKW = {"events", "http", "server", "location", "include", "internal", "on", "off"};
	return stKW.find(szKW)!=stKW.end();
}

bool isNginxKeyword2( const char* szKW ) {
	static set<string> stKW = {"auth_request", "auth_request_set", "auth_basic", "auth_basic_user_file",
		"add_header",  "client_max_body_size",
		"default_type", "sendfile", "listen", "server_name", "root", "alias", "index", "allow", "deny", "keepalive_timeout", "worker_connections", "worker_processes",
		"proxy_pass", "proxy_set_header", "proxy_pass_request_body",
		"proxy_cache", "proxy_cache_path", "proxy_cache_methods", "proxy_cache_key", "proxy_cache_valid",
		"fastcgi_pass", "fastcgi_connect_timeout", "fastcgi_param"};
	bool b = stKW.find(szKW)!=stKW.end();
	return b;
}

#define STYLE_TEXT			0
#define STYLE_KW1			1
#define STYLE_KW2			11
#define STYLE_COMMENT		2
#define STYLE_SYMBOL		3
#define STYLE_STRING		4
#define STYLE_CHAR			5
#define STYLE_NUMBER		6
#define STYLE_UNQ			7
#define STYLE_TAG			8
#define STYLE_VAR			9

%}

%%

[{}]			SetStyle(STYLE_SYMBOL); if( *yytext=='{' ) levelIncrease(); else levelDecrease();

[_a-zA-Z][_a-zA-Z0-9]*		{
				char chB = *(yytext-1);
				bool bB = true; // strchr(" \t\r\n",chB);
				int kw = 0;
				if( m_langId==1 ) {
					if( bB && isApacheKeyword(yytext) )
						kw = 1;
					else if( bB && isApacheKeyword2(yytext) )
						kw = 2;
				} else 
				if( m_langId==2 ) {
					if( bB && isNginxKeyword(yytext) )
						kw = 1;
					else if( bB && isNginxKeyword2(yytext) )
						kw = 2;
				}
				switch( kw ) {
					case 1: SetStyle(STYLE_KW1); break;
					case 2: SetStyle(STYLE_KW2); break;
					default: SetStyle(); break;
				}
}

$[_a-zA-Z][_a-zA-Z0-9]*			SetStyle(STYLE_VAR);	// nginx VAR

%\{[_a-zA-Z][_a-zA-Z0-9]*\}		SetStyle(STYLE_VAR);	// apache VAR

[0-9]+			SetStyle(STYLE_NUMBER);

^{SPACE}#\{\{		BEGIN(SLCOMMENT); levelIncrease(); SetStyle();

^{SPACE}#\}\}		BEGIN(SLCOMMENT); levelDecrease(); SetStyle();

#		BEGIN(SLCOMMENT); SetStyle();

\<			SetStyle(STYLE_SYMBOL);	BEGIN(TAG); levelIncrease();
\<\/		SetStyle(STYLE_SYMBOL);	BEGIN(TAG); levelDecrease();
\"			BEGIN(STRING); SetStyle();

<TAG>{
	\"		BEGIN(TAG_STRING); SetStyle();
	\>		SetStyle(STYLE_SYMBOL); BEGIN(INITIAL); 
}

<TAG_STRING,STRING>{
	\\\"	SetStyle();
	{EOL}	SetStyle(STYLE_UNQ); BEGIN(INITIAL);
}
<TAG_STRING>{
	\"	SetStyle(); BEGIN(TAG);
}
<STRING>{
	\"	SetStyle(); BEGIN(INITIAL);
}

<SLCOMMENT>{
	{EOL}	BEGIN(INITIAL); SetStyle( CSciStyler::styleEOL );
}

<*>{
	{EOL}						SetStyle( CSciStyler::styleEOL );
	 .	SetStyle();
}

%%

int CSciLexer<yyFlexLexer>::StateToStyle(int nState) {
	switch(nState) {
		case INITIAL:		return STYLE_TEXT;
		case SLCOMMENT:		return STYLE_COMMENT;
		case STRING:		return STYLE_STRING;
		case TAG:			return STYLE_TAG;
		case TAG_STRING:	return STYLE_STRING;
	}
	return 0;
}