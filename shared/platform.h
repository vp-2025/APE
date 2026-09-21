#pragma once

#include <string>
#include <map>
#include <windows.h>
#include "ctime.h"
#include "str.h"

inline std::string GetWinDir() {
	wstrbuf sBuf(MAX_PATH);
    UINT len = GetWindowsDirectoryW(sBuf.buf(), sBuf.size());
    return w2utf( sBuf.str(len) ); // len without ending zero
}
inline std::string GetCurDir() {
	wstrbuf sBuf(MAX_PATH);
    UINT len = GetCurrentDirectoryW(sBuf.size(), sBuf.buf());
    return w2utf( sBuf.str(len) ); // len without ending zero
}
inline void SetCurDir(const std::string& sDir) {
    SetCurrentDirectoryW(utf2w(sDir).c_str());
}

inline std::string ExtractFilePath(const std::string& s) {
    size_t pos = s.find_last_of("\\/");
    return (pos != std::string::npos) ? s.substr(0, pos + 1) : std::string();
}
inline std::string ExtractFileName(const std::string& s) {
    size_t pos = s.find_last_of("\\/");
    return (pos != std::string::npos) ? s.substr(pos + 1) : s;
}
inline std::string ExtractFileExt(const std::string& s) { // without .
    size_t pos = s.rfind('.');
    return (pos != std::string::npos) ? s.substr(pos + 1) : s;
}
inline std::string ExtractFileNameWithoutExt(const std::string& s1) {
    std::string s = ExtractFileName(s1);
    size_t pos = s.rfind('.');
    return (pos != std::string::npos) ? s.substr(0, pos) : s;
}

inline std::string GetExeFileName() {
    wstrbuf sBuf(MAX_PATH);
    UINT len = GetModuleFileNameW(nullptr, sBuf.buf(), sBuf.size());
    return w2utf( sBuf.str(len) ); // len without ending zero
}
inline std::string GetExePath() {
    return ExtractFilePath(GetExeFileName());
}

inline bool hasTailSlash(const std::string& sPath) {
    size_t len = sPath.size();
    return len && (sPath[len - 1] == '\\' || sPath[len - 1] == '/');
}
inline void addTailSlash(std::string& sPath) {
    if( !hasTailSlash(sPath) )
        sPath += '\\';
}
inline void delTailSlash(std::string& sPath) {
    if( hasTailSlash(sPath) )
        sPath.erase(sPath.end() - 1, sPath.end());
}

inline DWORD GetFileAttrs(const std::string& s) { return GetFileAttributesW(utf2w(s).c_str()); }
inline BOOL SetFileAttrs(const std::string& s, DWORD dw) { return SetFileAttributesW( utf2w(s).c_str(), dw ); }

inline bool IsFileExists(const std::string& s) {
    DWORD dw = GetFileAttrs(s);
    return dw != INVALID_FILE_ATTRIBUTES && (dw & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

inline bool IsDirExists(const std::string& s) {
    DWORD dw = GetFileAttrs(s);
    return dw != INVALID_FILE_ATTRIBUTES && dw & FILE_ATTRIBUTE_DIRECTORY;
}

int64_t vGetFileSize(const std::string& sFile);
CTime vGetFileTime(const std::string& sFile);
bool vGetFileTimeSize(const std::string& sFile, CTime& tm, int64_t& size);
std::string vFileAttrs2Str(const std::string& sFile);

std::string GetLastErrorText();

void MsgBox(const char* szMsg);
void MsgBoxError(const char* szMsg);
void MsgBox(const wchar_t* szMsg);
void MsgBoxError(const wchar_t* szMsg);
inline void MsgBox(const std::string& s) { MsgBox(s.c_str()); }
inline void MsgBoxError(const std::string& s) { MsgBoxError(s.c_str()); }
inline void MsgBox(const std::wstring& s) { MsgBox(s.c_str()); }
inline void MsgBoxError(const std::wstring& s) { MsgBoxError(s.c_str()); }

std::string clr2str(int clr);
int str2clr(const std::string& str);

#define clrBlack    RGB(0,0,0)
#define clrWhite    RGB(0xFF,0xFF,0xFF)

inline bool isDarkColor(DWORD clr) {
    return GetRValue(clr) + GetGValue(clr) + GetBValue(clr) <= 255;
}

class CBrushCache {
    static std::map<DWORD, HBRUSH> mBr;
public:
    static HBRUSH color2brush(DWORD clr) {
        if( mBr.find(clr) == mBr.end() )
            mBr[clr] = CreateSolidBrush(clr);
        return mBr[clr];
    }
    static void clear() {
        for( auto it: mBr )
            DeleteObject(it.second);
        mBr.clear();
    }
};

bool LoadTextFromFile(const std::string& szFile, std::string& sText);
std::string LoadTextFromResource(const char* szResID);

void EnsureFilePath(std::string& sFile, std::string sPath = ""); // if file has no path, then CurDir is used
bool EqualsFilePath(std::string sFile1, std::string sFile2);

bool CreateProcess(const std::string& s);
bool CreateProcessW(const std::wstring& s);

bool dlgOpenFile(HWND hWnd, std::string& sFile);
bool dlgSaveFile(HWND hWnd, std::string& sFile);
bool dlgChooseColor(HWND hWnd, int& clr);

BOOL HandleDialogTooltips(NMHDR* pNMHDR, HMENU hMenu);