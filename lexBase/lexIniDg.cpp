#include "stdafx.h"
#include "lexIniDg.h"
#include "sci.h"
#include "shared/str.h"
#include "shared/platform.h"

#include <algorithm>
#include <map>
#include <memory>
using namespace std;

const CIniDgSchema* g_pIniDgLexing = nullptr;

/////////////////////////////////////////////////////////////////////
// names

static bool isValidUtf8(const string& s) {
    for( size_t i = 0; i < s.size(); ) {
        unsigned char c = s[i];
        int n = c < 0x80 ? 0 : (c & 0xE0) == 0xC0 ? 1 : (c & 0xF0) == 0xE0 ? 2 : (c & 0xF8) == 0xF0 ? 3 : -1;
        if( n < 0 || i + n >= s.size() + (n ? 0 : 1) )
            return false;
        for( int k = 1; k <= n; k++ )
            if( ((unsigned char)s[i + k] & 0xC0) != 0x80 )
                return false;
        i += n + 1;
    }
    return true;
}

// the lookup key of a name: trimmed, utf-8 (an ANSI file has "Großrechner" in cp1252), ASCII lower-case
static string normName(const char* p, size_t n) {
    while( n && (*p == ' ' || *p == '\t') ) p++, n--;
    while( n && (p[n - 1] == ' ' || p[n - 1] == '\t') ) n--;
    string s(p, n);
    if( !isValidUtf8(s) )
        s = a2utf(s);
    for( auto& ch : s )
        if( ch >= 'A' && ch <= 'Z' )
            ch = ch - 'A' + 'a';
    return s;
}
static string normName(const string& s) { return normName(s.data(), s.size()); }

// a family name: <x> is one or more chars, [..] is optional (nested); both are lower-case
static bool matchPattern(const string& p, size_t pi, const string& s, size_t si) {
    while( pi < p.size() ) {
        char c = p[pi];
        if( c == '<' ) {
            size_t pe = p.find('>', pi);
            if( pe == string::npos )
                return false;
            for( size_t k = si + 1; k <= s.size(); k++ )
                if( matchPattern(p, pe + 1, s, k) )
                    return true;
            return false;
        }
        if( c == '[' ) {
            size_t pe = pi;
            for( int depth = 0; pe < p.size(); pe++ ) {
                if( p[pe] == '[' ) depth++;
                if( p[pe] == ']' && --depth == 0 ) break;
            }
            string rest = pe < p.size() ? p.substr(pe + 1) : string();
            string with = p.substr(0, pi) + p.substr(pi + 1, pe - pi - 1) + rest;
            string without = p.substr(0, pi) + rest;
            return matchPattern(with, pi, s, si) || matchPattern(without, pi, s, si);
        }
        if( si >= s.size() || s[si] != c )
            return false;
        pi++, si++;
    }
    return si == s.size();
}

const CIniDgKey* CIniDgSection::Find(const string& sKey) const {
    string sKeyNorm = normName(sKey);
    auto it = index.find(sKeyNorm);
    if( it != index.end() )
        return &keys[it->second];
    for( int i : patterns )
        if( matchPattern(normName(keys[i].sName), 0, sKeyNorm, 0) )
            return &keys[i];
    return nullptr;
}

int CIniDgSchema::FindSection(const string& sSection) const {
    auto it = index.find(normName(sSection));
    return it != index.end() ? it->second : -1;
}

/////////////////////////////////////////////////////////////////////
// the meta file

static string trimStr(const string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if( b == string::npos )
        return string();
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

bool CIniDgSchema::Load(const string& sMetaFile) {
    string sText;
    if( !LoadTextFromFile(sMetaFile, sText) )
        return false;
    if( sText.compare(0, 3, "\xEF\xBB\xBF") == 0 )
        sText.erase(0, 3);
    if( !isValidUtf8(sText) )
        sText = a2utf(sText);
    sFile = sMetaFile;
    sections.clear();
    index.clear();

    int iSec = -1; // the current section (an index: sections grows)
    bool bAfterHeader = false;
    size_t pos = 0;
    while( pos < sText.size() ) {
        size_t eol = sText.find('\n', pos);
        if( eol == string::npos )
            eol = sText.size();
        string sLine = trimStr(sText.substr(pos, eol - pos));
        pos = eol + 1;
        if( sLine.empty() )
            continue;
        if( sLine[0] == ';' || sLine[0] == '#' ) {
            if( bAfterHeader && iSec >= 0 && sections[iSec].sDesc.empty() )
                sections[iSec].sDesc = trimStr(sLine.substr(1));
            bAfterHeader = false;
            continue;
        }
        bAfterHeader = false;
        if( sLine[0] == '[' ) {
            size_t e = sLine.find(']');
            string sSection = trimStr(sLine.substr(1, e == string::npos ? string::npos : e - 1));
            string sSectionNorm = normName(sSection);
            auto it = index.find(sSectionNorm);
            if( it == index.end() ) { // a section can be listed twice: its keys are merged
                iSec = (int)sections.size();
                index[sSectionNorm] = iSec;
                sections.emplace_back();
                sections.back().sName = sSection;
            } else
                iSec = it->second;
            bAfterHeader = true;
            continue;
        }
        size_t eq = sLine.find('=');
        if( iSec < 0 || eq == string::npos )
            continue;
        CIniDgSection& sec = sections[iSec];
        CIniDgKey key;
        key.sName = trimStr(sLine.substr(0, eq));
        string sRest = trimStr(sLine.substr(eq + 1));
        // default;description - "..." and `...` can contain ';'
        size_t semi = string::npos;
        if( !sRest.empty() && (sRest[0] == '"' || sRest[0] == '`') ) {
            size_t q = sRest.find(sRest[0], 1);
            if( q != string::npos )
                semi = sRest.find(';', q);
        } else
            semi = sRest.find(';');
        key.sDefault = trimStr(sRest.substr(0, semi));
        if( key.sDefault.size() >= 2 && key.sDefault[0] == '"' && key.sDefault.back() == '"' )
            key.sDefault = key.sDefault.substr(1, key.sDefault.size() - 2);
        if( semi != string::npos )
            key.sDesc = trimStr(sRest.substr(semi + 1));
        if( key.sName.empty() )
            continue;

        string sKeyNorm = normName(key.sName);
        bool bPattern = key.sName.find_first_of("<[") != string::npos;
        if( !bPattern && sec.index.count(sKeyNorm) )
            continue;
        int i = (int)sec.keys.size();
        sec.keys.push_back(key);
        if( bPattern )
            sec.patterns.push_back(i);
        else
            sec.index[sKeyNorm] = i;
    }
    return true;
}

/////////////////////////////////////////////////////////////////////
// the schema of an ini file, loaded again when the meta file is changed

struct CSchemaCache {
    FILETIME ft{};
    unique_ptr<CIniDgSchema> pSchema;
};

static bool getWriteTime(const string& sFile, FILETIME& ft) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if( !GetFileAttributesExW(utf2w(sFile).c_str(), GetFileExInfoStandard, &fad) )
        return false;
    ft = fad.ftLastWriteTime;
    return true;
}

const CIniDgSchema* iniDgSchemaFor(const string& sFile) {
    static map<string, CSchemaCache> cache; // meta file -> schema

    string sName = ExtractFileName(sFile);
    size_t dot = sName.rfind('.');
    if( dot == string::npos || dot == 0 )
        return nullptr;
    string sBase = sName.substr(0, dot);
    string sBaseNorm = normName(sBase);
    if( normName(sName.substr(dot)) != ".ini" )
        return nullptr;
    if( sBaseNorm.size() > 3 && sBaseNorm.compare(sBaseNorm.size() - 3, 3, "-dg") == 0 )
        return nullptr; // the meta files themselves are plain ini files
    string sMeta = sBase + "-dg.ini";

    string sExe = GetExePath();
    string dirs[] = {ExtractFilePath(sFile), sExe};
    for( const string& sDir : dirs ) {
        if( sDir.empty() )
            continue;
        string sPath = sDir + sMeta;
        FILETIME ft;
        if( !getWriteTime(sPath, ft) )
            continue;
        CSchemaCache& c = cache[normName(sPath)];
        if( !c.pSchema || CompareFileTime(&c.ft, &ft) != 0 ) {
            auto p = make_unique<CIniDgSchema>();
            if( !p->Load(sPath) )
                continue;
            c.pSchema = std::move(p);
            c.ft = ft;
        }
        return c.pSchema.get();
    }
    return nullptr;
}

/////////////////////////////////////////////////////////////////////
// editor support

// the text in the document's encoding: the schema is utf-8
static string toDoc(CSciWrapper& sci, const string& s) {
    return sci.isEncodingUtf() ? s : utf2a(s);
}

static string toUtf(const string& s) {
    return isValidUtf8(s) ? s : a2utf(s);
}

static string lineText(CSciWrapper& sci, int line) {
    return sci.GetTextRange(sci.PositionFromLine(line), sci.GetLineEndPosition(line));
}

// the section of the line: the last [header] above it; "" before the first one
static void currentSection(CSciWrapper& sci, int line, string& sSection) {
    for( ; line >= 0; line-- ) {
        string s = trimStr(lineText(sci, line));
        if( s.size() >= 2 && s[0] == '[' ) {
            size_t e = s.find(']');
            sSection = trimStr(s.substr(1, e == string::npos ? string::npos : e - 1));
            return;
        }
    }
    sSection.clear();
}

static bool isKeyChar(int ch) {
    return ch > ' ' && ch != '=' && ch != ':' && ch != ';' && ch != '#' && ch != '[' && ch != ']';
}

// the line up to the caret is "  key-prefix": its start is in posKey
static bool atKeyStart(CSciWrapper& sci, int pos, int& posKey) {
    int line = sci.LineFromPosition(pos);
    int start = sci.PositionFromLine(line);
    string s = sci.GetTextRange(start, pos);
    size_t b = s.find_first_not_of(" \t");
    if( b == string::npos )
        b = s.size();
    for( size_t i = b; i < s.size(); i++ )
        if( !isKeyChar((unsigned char)s[i]) )
            return false;
    posKey = start + (int)b;
    return true;
}

// the line up to the caret is "  [section-prefix"
static bool atSectionStart(CSciWrapper& sci, int pos, int& posName) {
    int line = sci.LineFromPosition(pos);
    int start = sci.PositionFromLine(line);
    string s = sci.GetTextRange(start, pos);
    size_t b = s.find_first_not_of(" \t");
    if( b == string::npos || s[b] != '[' || s.find(']', b) != string::npos )
        return false;
    posName = start + (int)b + 1;
    return true;
}

static void showList(CSciWrapper& sci, vector<string> items, int lenEntered) {
    sort(items.begin(), items.end(), [](const string& a, const string& b) { return normName(a) < normName(b); });
    items.erase(unique(items.begin(), items.end(), [](const string& a, const string& b) { return normName(a) == normName(b); }), items.end());
    string sList;
    for( const auto& s : items ) {
        if( !sList.empty() )
            sList += ' ';
        sList += toDoc(sci, s);
    }
    if( sList.empty() )
        return;
    sci.AutoCSetIgnoreCase(true);
//    sci.AutoCSetOrder(SC_ORDER_PERFORMSORT);
    sci.AutoCShow(lenEntered, sList);
}

bool iniDgAutoComplete(CSciWrapper& sci, const string& sFile) {
    const CIniDgSchema* pSchema = iniDgSchemaFor(sFile);
    if( !pSchema )
        return false;
    int pos = sci.GetCurrentPos(), posStart;
    vector<string> items;
    if( atSectionStart(sci, pos, posStart) ) {
        for( const auto& sec : pSchema->sections )
            items.push_back(sec.sName);
        showList(sci, items, pos - posStart);
    } else
    if( atKeyStart(sci, pos, posStart) ) {
        string sSection;
        currentSection(sci, sci.LineFromPosition(pos), sSection);
        int iSec = pSchema->FindSection(sSection);
        if( iSec < 0 )
            return true;
        for( const auto& key : pSchema->sections[iSec].keys )
            items.push_back(key.sName);
        showList(sci, items, pos - posStart);
    }
    return true;
}

// the text cut into lines of max ~width chars at spaces
static string wrapText(const string& s, size_t width = 90) {
    string sOut;
    size_t lineLen = 0;
    size_t i = 0;
    while( i < s.size() ) {
        size_t e = s.find(' ', i);
        if( e == string::npos )
            e = s.size();
        string sWord = s.substr(i, e - i);
        if( lineLen && lineLen + 1 + sWord.size() > width ) {
            sOut += '\n';
            lineLen = 0;
        } else
        if( lineLen ) {
            sOut += ' ';
            lineLen++;
        }
        sOut += sWord;
        lineLen += sWord.size();
        i = e + 1;
    }
    return sOut;
}

bool iniDgCallTip(CSciWrapper& sci, const string& sFile) {
    const CIniDgSchema* pSchema = iniDgSchemaFor(sFile);
    if( !pSchema )
        return false;
    sci.CalltipCancel();
    int pos = sci.GetCurrentPos();
    int line = sci.LineFromPosition(pos);
    int start = sci.PositionFromLine(line);
    string sLine = lineText(sci, line);
    size_t b = sLine.find_first_not_of(" \t");
    if( b == string::npos || sLine[b] == ';' || sLine[b] == '#' )
        return true;

    string sTip;
    if( sLine[b] == '[' ) {
        size_t e = sLine.find(']', b);
        string sName = trimStr(sLine.substr(b + 1, e == string::npos ? string::npos : e - b - 1));
        int iSec = pSchema->FindSection(sName);
        if( iSec < 0 )
            sTip = "Unknown section [" + toUtf(sName) + "] - not in " + ExtractFileName(pSchema->sFile);
        else {
            const CIniDgSection& sec = pSchema->sections[iSec];
            sTip = "[" + sec.sName + "]  " + to_string(sec.keys.size()) + " keys";
            if( !sec.sDesc.empty() )
                sTip += "\n" + wrapText(sec.sDesc);
        }
    } else {
        size_t e = sLine.find_first_of("=:", b);
        string sKey = trimStr(sLine.substr(b, e == string::npos ? string::npos : e - b));
        if( sKey.empty() )
            return true;
        string sSection;
        currentSection(sci, line, sSection);
        int iSec = pSchema->FindSection(sSection);
        const CIniDgKey* pKey = iSec >= 0 ? pSchema->sections[iSec].Find(sKey) : nullptr;
        if( pKey ) {
            sTip = pKey->sName + " = " + pKey->sDefault;
            if( !pKey->sDesc.empty() )
                sTip += "\n" + wrapText(pKey->sDesc);
        } else {
            sTip = sSection.empty() ? "Key " + toUtf(sKey) + " outside of a section"
                                    : "Unknown key " + toUtf(sKey) + " in [" + toUtf(sSection) + "]";
            string sOther; // the sections, which know it
            for( const auto& sec : pSchema->sections )
                if( sec.Find(sKey) )
                    sOther += (sOther.empty() ? "[" : ", [") + sec.sName + "]";
            if( !sOther.empty() )
                sTip += "\nknown in " + sOther;
        }
    }
    sci.CalltipShow(start + (int)b, toDoc(sci, sTip));
    return true;
}

void iniDgCharAdded(CSciWrapper& sci, const string& sFile, int ch) {
    if( !iniDgSchemaFor(sFile) )
        return;
    if( ch == '=' ) {
        iniDgCallTip(sci, sFile);
        return;
    }
    if( sci.AutoCActive() )
        return;
    int pos = sci.GetCurrentPos(), posStart;
    if( ch == '[' ) {
        if( atSectionStart(sci, pos, posStart) && pos == posStart )
            iniDgAutoComplete(sci, sFile);
    } else
    if( isKeyChar(ch) && atKeyStart(sci, pos, posStart) && pos - posStart == 1 )
        iniDgAutoComplete(sci, sFile);
}
