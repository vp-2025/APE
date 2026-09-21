#pragma once

#include <string>
#include <vector>
#include <set>
#include <windows.h> // CP_ACP

inline void toLower(std::string& s) {
	for( char& ch: s )
		if( std::isupper((unsigned char) ch) )
			ch = (char) std::tolower(ch);
}
inline void toUpper(std::string& s) {
	for( char& ch: s )
		if( std::islower((unsigned char) ch) )
			ch = (char) std::toupper(ch);
}

inline std::string toLowerVal(std::string s) {
	toLower(s);
	return s;
}
inline std::string toUpperVal(std::string s) {
	toUpper(s);
	return s;
}

inline bool startsWith(const std::string& str, const std::string& s) {
    return str.size() >= s.size() && strncmp(str.c_str(), s.c_str(), s.size()) == 0;
}
inline bool endsWith(const std::string& str, const std::string& s) {
    return str.size() >= s.size() && strncmp(str.c_str() + str.size() - s.size(), s.c_str(), s.size()) == 0;
}
inline bool equalsTo(const std::string& str, const std::string& s) {
    return str.size() == s.size() && strcmp(str.c_str(), s.c_str()) == 0;
}
inline int compareTo(const std::string& str, const std::string& s) {
    return strcmp(str.c_str(), s.c_str());
}
inline bool contains(const std::string& str, const std::string& s) {
    return str.find(s) != std::string::npos;
}
inline bool contains(const std::string& str, const char* sz) {
    return str.find(sz) != std::string::npos;
}

inline bool startsWithIC(const std::string& str, const std::string& s) {
    return str.size() >= s.size() && strnicmp(str.c_str(), s.c_str(), s.size()) == 0;
}
inline bool endsWithIC(const std::string& str, const std::string& s) {
    return str.size() >= s.size() && strnicmp(str.c_str() + str.size() - s.size(), s.c_str(), s.size()) == 0;
}
inline bool equalsToIC(const std::string& str, const std::string& s) {
    return str.size() == s.size() && stricmp(str.c_str(), s.c_str()) == 0;
}
inline int compareToIC(const std::string& str, const std::string& s) {
    return stricmp(str.c_str(), s.c_str());
}
inline bool containsIC(const std::string& str, const std::string& s) {
	return toLowerVal(str).find(toLowerVal(s)) != std::string::npos;
}
inline bool containsIC(const std::string& str, const char* sz) {
	return toLowerVal(str).find(toLowerVal(sz)) != std::string::npos;
}

void split(const std::string& str, std::vector<std::string>& v, char ch);
void split(const std::string& str, std::set<std::string>& st, char ch);
void splitQ(const std::string& str, std::vector<std::string>& v, char ch);
std::string combine(const std::vector<std::string>& v, char ch);
std::string combine(const std::set<std::string>& st, char ch);

std::wstring utf2w(const std::string& s);
std::string w2utf(const std::wstring& ws);
std::wstring a2w(const std::string& s, int cp = CP_ACP);
std::string w2a(const std::wstring& ws, int cp = CP_ACP);

inline std::string utf2a(const std::string& s, int cp = CP_ACP) {
    std::wstring ws = utf2w(s);
    return w2a(ws, cp);
}
inline std::string a2utf(const std::string& s, int cp = CP_ACP) {
    std::wstring ws = a2w(s, cp);
    return w2utf(ws);
}

#ifdef UNICODE
#define t2a(s)		w2a(s)
#define t2utf(s)	w2utf(s)
#define t2w(s)		s

#define a2t(s)		a2w(s)
#define utf2t(s)	utf2w(s)
#define w2t(s)		s
#else
#define t2a(s)        s
#define t2utf(s)      a2utf(s)
#define t2w(s)        a2w(s)

#define a2t(s)        s
#define utf2t(s)      utf2a(s)
#define w2t(s)        w2a(s)
#endif

template<class T>
int replaceStr(T& str, const char* szFrom, const char* szTo) {
    T res;
    res.reserve(str.size());
    size_t pos0 = 0;
    int cnt = 0;
    while( true ) {
        size_t pos = str.find(szFrom, pos0);
        if( pos != std::string::npos ) {
            res += str.substr(pos0, pos - pos0);
            res += szTo;
            pos0 = pos + strlen(szFrom);
            ++cnt;
        } else {
            res += str.substr(pos0);
            break;
        }
    }
    str = res;
    return cnt;
}

void trim(std::string& s, bool bLeft = true, bool bRight = true);
inline void ltrim(std::string& s) { trim(s, true, false); }
inline void rtrim(std::string& s) { trim(s, false, true); }
inline std::string trimc( std::string s ) { trim(s); return s; }

// string&
inline std::string& operator<<(std::string& str, const std::string& s) {
    str += s;
    return str;
}
inline std::string& operator<<(std::string& str, const char* sz) {
    str += sz;
    return str;
}
inline std::string& operator<<(std::string& str, char ch) {
    str += ch;
    return str;
}
inline std::string& operator<<(std::string& str, int i) {
    return str << std::to_string(i);
}
inline std::string& operator<<(std::string& str, unsigned ui) {
    return str << std::to_string(ui);
}
inline std::string& operator<<(std::string& str, long l) {
    return str << std::to_string(l);
}
inline std::string& operator<<(std::string& str, unsigned long ul) {
    return str << std::to_string(ul);
}
inline std::string& operator<<(std::string& str, int64_t i) {
    return str << std::to_string(i);
}

// wstring&
inline std::wstring& operator<<(std::wstring& wstr, const std::wstring& ws) {
    wstr += ws;
    return wstr;
}
inline std::wstring& operator<<(std::wstring& wstr, const wchar_t* wsz) {
    wstr += wsz;
    return wstr;
}
inline std::wstring& operator<<(std::wstring& wstr, const std::string& s) {
    return wstr << utf2w(s);
}
inline std::wstring& operator<<(std::wstring& wstr, const char* sz) {
    return wstr << utf2w(sz);
}
inline std::wstring& operator<<(std::wstring& str, int i) {
	return str << std::to_string(i);
}
inline std::wstring& operator<<(std::wstring& str, int64_t i) {
	return str << std::to_string(i);
}

std::string Format(const char* szFormat, ...);
std::wstring Format(const wchar_t* szFormat, ...);

//////////////////////////////////////////////////////////////////
void MsgBox(const std::string& s);

template<class T>
class basic_strbuf {
	unsigned int m_len{};
	T* m_buf;
public:
	basic_strbuf() { m_len=0; m_buf=nullptr; }
	explicit basic_strbuf(unsigned int len) { init(len); }
	~basic_strbuf() { delete[] m_buf; }

	void init(unsigned int len) { m_len=len+1; m_buf=new T[m_len]; m_buf[0]=0; } // +1 is for ending zero
	void set(const T* sz) {
		int i=0;
		for( ; sz[i] && i<m_len; ++i)
			m_buf[i] = sz[i];
		m_buf[i] = 0;
	}
	void set(const std::basic_string<T>& s) { set(s.c_str()); }

	T* buf() { return m_buf; }
	int size() const { return m_len; }

	std::basic_string<T> str(unsigned int len) {
        size_t len2 = length();
		if( len!=len2 ) {
            MsgBox( "len!=len2: "+std::to_string(len)+'>'+std::to_string(len2) );
        }
        if( len>m_len ) {
            MsgBox("len>m_len: "+std::to_string(len)+'>'+std::to_string(m_len));
            len = m_len;
        }
		return std::basic_string<T>(m_buf,len);
	}
	std::basic_string<T> str() {
        size_t len = length();
        if( len>m_len ) {
            MsgBox("len>m_len: "+std::to_string(len)+'>'+std::to_string(m_len));
            len = m_len;
        }
        return std::basic_string<T>(m_buf,len);
    }

private:
    size_t length() const {
        size_t len=0;
        while( m_buf[len] && len<m_len  )
            ++len;
        return len;
    }
};
using strbuf  = basic_strbuf<char>;
using wstrbuf  = basic_strbuf<wchar_t>;