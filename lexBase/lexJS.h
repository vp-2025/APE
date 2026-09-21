#pragma once

#include <string>
#include "sci.h"

void jsGetRootList( string& str );
void jsGetRootParams( const string& sMember, string& sPrm, string& sDesc );
void jsGetObjectList( const string& sObject, string& str );
void jsGetObjectParams( const string& sObject, const string& sMember, string& sPrm, string& sDesc );

extern bool g_isJSRegExp;
bool isJSKeyword( const char* szKW );

void jsFix4JQuery( string& sObject, bool bDot );
bool isJSSkipStyle(CSciWrapper& sci, int pos, bool bStringChar=true);

bool isJSCode( CSciWrapper& sci );