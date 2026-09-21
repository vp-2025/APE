#pragma once

#include <string>
#include "sci.h"

// autocomplete
string cssGetKeywordList();
string cssGetValueList( const string& sKeyword );

// calltip
string cssGetKeywordDesc( const string& sKeyword );
string cssGetValueDesc( const string& sKeyword, const string& sValue );

// show as bold
bool isCssKeyword( const char* sz );
bool isCssValue( const char* sz );

bool isCSSAttr( CSciWrapper& sci, int pos=-1 );
bool isCSSValue( CSciWrapper& sci, int pos=-1 );

string cssGetKeyword( CSciWrapper& sci );
