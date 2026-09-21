#include "str.h"
#include <windows.h>

using namespace std;

wstring a2w(const string& s, int cp) {
    int len = MultiByteToWideChar(cp, 0, s.c_str(), -1, nullptr, 0);
	wstrbuf sBuf(len); // len includes ending zero
    len = MultiByteToWideChar(cp, 0, s.c_str(), -1, sBuf.buf(), sBuf.size());
    return sBuf.str(len-1); // -1 is ending zero
}

wstring utf2w(const string& s) {
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	wstrbuf sBuf(len); // len includes ending zero
    len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, sBuf.buf(), sBuf.size());
    return sBuf.str(len-1); // -1 is ending zero
}

string w2a(const wstring& ws, int cp) {
    int len = WideCharToMultiByte(cp, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    strbuf sBuf(len); // len includes ending zero
    len = WideCharToMultiByte(cp, 0, ws.c_str(), -1, sBuf.buf(), sBuf.size(), nullptr, nullptr);
    return sBuf.str(len-1); // -1 is ending zero
}

string w2utf(const wstring& ws) {
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    strbuf sBuf(len); // len includes ending zero
    len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, sBuf.buf(), sBuf.size(), nullptr, nullptr);
    return sBuf.str(len-1); // -1 is ending zero
}

string FormatV(const char* szFormat, va_list ArgList) {
    int len = vsnprintf(nullptr, 0, szFormat, ArgList);
    strbuf sBuf( len+1 ); // +1 for ending zero
    len = vsnprintf(sBuf.buf(), sBuf.size(), szFormat, ArgList);
    return sBuf.str(len); // len without ending zero
}

wstring FormatV(const wchar_t* szFormat, va_list ArgList) {
    int len = vswprintf(nullptr, 0, szFormat, ArgList)+1; // because 0 at end
    wstrbuf sBuf( len+1 ); // +1 for ending zero
    len = vswprintf(sBuf.buf(), sBuf.size(), szFormat, ArgList);
    return sBuf.str(len); // len without ending zero
}

string Format(const char* szFormat, ...) {
    string s;
    va_list argList;
        va_start(argList, szFormat);
    s = FormatV(szFormat, argList);
        va_end(argList);
    return s;
}

wstring Format(const wchar_t* szFormat, ...) {
    wstring ws;
    va_list argList;
        va_start(argList, szFormat);
    ws = FormatV(szFormat, argList);
        va_end(argList);
    return ws;
}

void split(const string& str, vector<string>& v, char chSep) {
	v.clear();
	size_t k, k0 = 0;
	while( (k = str.find(chSep, k0)) != string::npos ) {
		v.push_back( str.substr(k0, k - k0) );
		k0 = k + 1;
	}
	if( k0 != string::npos && k0 < str.length() )
		v.push_back( str.substr(k0) );
}

void split(const string& str, set<string>& st, char chSep) {
	st.clear();
	size_t k, k0 = 0;
	while( (k = str.find(chSep, k0)) != string::npos ) {
		st.insert( str.substr(k0, k - k0) );
		k0 = k + 1;
	}
	if( k0 != string::npos && k0 < str.length() )
		st.insert( str.substr(k0) );
}

size_t findQ(const string& str, char ch, size_t pos) {
	bool bQ = false;
	for( size_t i = pos; i < str.length(); ++i ) {
		if( !bQ && str[i] == ch )
			return i;
		if( str[i] == '"' )
			bQ = !bQ;
	}
	return string::npos;
}

string trimQ(const string& str) {
	if( str.size()>2 && str[0]=='"' && str[str.size()-1]=='"' )
		return str.substr(1,str.size()-2);
	return str;
}

void splitQ(const string& str, vector<string>& v, char chSep) {
    v.clear();
    size_t k, k0 = 0;
    while( (k = findQ(str, chSep, k0)) != string::npos ) {
        v.push_back( trimQ( str.substr(k0, k - k0) ) );
        k0 = k + 1;
    }
    if( k0 != string::npos && k0 < str.length() )
        v.push_back( trimQ( str.substr(k0) ) );
}

string combine(const vector<string>& list, char chSep) {
    string str;
    for( const string& s: list ) {
        if( !str.empty() ) str += chSep;
        str += s;
    }
    return str;
}

string combine(const set<string>& list, char chSep) {
    string str;
    for( const string& s: list ) {
        if( !str.empty() ) str += chSep;
        str += s;
    }
    return str;
}

#define TRIM_CHARS "\r\n \t"

void trim(string& s, bool bLeft, bool bRight) {
    if( s.empty() )
        return;
    size_t b = bLeft ? s.find_first_not_of(TRIM_CHARS) : 0;
    size_t e = bRight ? s.find_last_not_of(TRIM_CHARS) : s.length() - 1;
    if( b == string::npos ) // No non-spaces
        return s.clear();
    if( b == 0 && e == s.length() - 1 )
        return;
    s = s.substr(b, e - b + 1);
}