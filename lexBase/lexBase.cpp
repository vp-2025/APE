#include "stdafx.h"
#include "LexBase.h"
using namespace std;

#define SEPARATORS " \r\n\t"

size_t find_one_of( const string& s, const char* sz, size_t pos )
{
	while( pos<=s.length() && !strchr(sz,s[pos]) )
		pos++;
	return pos>=s.length() ? string::npos : pos;
}

size_t find_one_not_of( const string& s, const char* sz, size_t pos )
{
	while( pos<=s.length() && strchr(sz,s[pos]) )
		pos++;
	return pos>=s.length() ? string::npos : pos;
}

void CKeywords::setText( const string& s, bool bIC )
{
	if( s.empty() ) return;

	size_t k1 = 0;
	while( true )
	{
		size_t k2 = find_one_of(s,SEPARATORS,k1);
		if( k2==string::npos )
		{
			st.insert( bIC ? toLowerVal(s.substr(k1)) : s.substr(k1) );
			break;
		} else {
			st.insert( bIC ? toLowerVal(s.substr(k1,k2-k1)) : s.substr(k1,k2-k1) );
			k1 = find_one_not_of(s,SEPARATORS,k2+1);
			if( k1==string::npos )
				break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////
// checkPreprocessor, used in: CPP, IDL, MAKE, ESQL

int strcmpIC( const char* sz1, const char* sz2, bool bIgnoreCase ) {
	return bIgnoreCase ? strcmpi(sz1, sz2) : strcmp(sz1, sz2);
}

void checkPreprocessor( const char* text, int lang, bool& bDec, bool& bInc, bool bIC/*=false*/ ) {
	bDec = bInc = false;
	if( lang==2 ) { // C#
		if( !strncmp(text,"region",6) ) bInc=true;
		if( !strncmp(text,"endregion",9) ) bDec=true;
	}

	char ch = text[0];
	if( bIC ) ch = tolower(ch);

	switch( ch ) {
		case 'i': 
			if( !strcmpIC(text,"if",bIC) || 
				!strcmpIC(text,"ifdef",bIC) || 
				!strcmpIC(text,"ifndef",bIC) )
				bInc = true;
			break;
		case 'e':
			if( !strcmpIC(text,"endif",bIC) ) bDec = true;
			if( !strcmpIC(text,"elseif",bIC) || 
				!strcmpIC(text,"elif",bIC) || 
				!strcmpIC(text,"else",bIC) ) { 
				bDec=true; 
				bInc=true; 
			}
			break;
	}
}