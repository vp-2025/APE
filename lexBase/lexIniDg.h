#pragma once

// Ini files described by a "-dg.ini" meta file: mea.ini <- mea-dg.ini, ablesen.ini <- ablesen-dg.ini.
// The meta file lists per section the known keys:   key = default;description
//   a key with <x> is a family of keys: ObisVector<n> = ObisVector0, ObisVector1 ...; [..] is optional
// The ini lexer (dialect LANG_DG) marks unknown sections and keys unknown in their section,
// the editor completes sections and keys of the current section and shows their default and description.

#include <string>
#include <vector>
#include <unordered_map>

class CSciWrapper;

struct CIniDgKey {
    std::string sName;    // as in the meta file: "ObisVector<n>"
    std::string sDefault;
    std::string sDesc;    // "[bool] ..."
};

struct CIniDgSection {
    std::string sName;
    std::string sDesc;    // the comment after the section header
    std::vector<CIniDgKey> keys;
    std::unordered_map<std::string, int> index; // lower-case name -> index in keys
    std::vector<int> patterns;                   // the families: <x> and [..] in the name

    const CIniDgKey* Find(const std::string& sKey) const;
};

struct CIniDgSchema {
    std::string sFile;    // the meta file
    std::vector<CIniDgSection> sections;
    std::unordered_map<std::string, int> index; // lower-case name -> index in sections

    int FindSection(const std::string& sSection) const; // -1: unknown
    bool Load(const std::string& sMetaFile);
};

// the schema of an ini file: <name>-dg.ini in the folder of the file, in <exe>\ini\ or <exe>\..\ini\; nullptr: none
const CIniDgSchema* iniDgSchemaFor(const std::string& sFile);

// the schema the ini lexer uses while styling (set before CLexerIni::Style)
extern const CIniDgSchema* g_pIniDgLexing;

// editor support; false: the file has no schema
bool iniDgAutoComplete(CSciWrapper& sci, const std::string& sFile);
bool iniDgCallTip(CSciWrapper& sci, const std::string& sFile);
void iniDgCharAdded(CSciWrapper& sci, const std::string& sFile, int ch); // shows the list or the calltip
