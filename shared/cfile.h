#pragma once

#include <windows.h>
#include <string>

enum CEncoding {
    encNone,
    encAnsi,
    encUtf8,
    encUtf8NoBOM,
    encUtf16BE, // BigEndian
    encUtf16LE, // LittleEndian
    encLatin1,
    encLatin2,
    encLatin3,
	encAnsiCyr
};

extern const char bomUtf8[];
extern const char bomUtf16BE[];
extern const char bomUtf16LE[];

class CFile {
    HANDLE hFile;
public:
    CFile() { hFile = INVALID_HANDLE_VALUE; }
    ~CFile() { Close(); }
    bool IsOpen() { return hFile != INVALID_HANDLE_VALUE; }
    bool Open(const std::string& sFile, bool bRead);
    DWORD Read(void* buf, DWORD len) {
        if( !IsOpen() ) return 0;
        DWORD dw = 0;
        ReadFile(hFile, buf, len, &dw, nullptr);
        return dw;
    }
    DWORD Write(const void* buf, DWORD len) {
        if( !IsOpen() ) return 0;
        DWORD dw = 0;
        WriteFile(hFile, buf, len, &dw, nullptr);
        return dw;
    }
    DWORD Write(const std::string& s) {
        return Write(s.c_str(), s.size());
    }
    DWORD Write(const std::wstring& s) {
        return Write(s.c_str(), s.size() * 2);
    }
    bool Close() {
        if( !IsOpen() ) return false;
        BOOL b = CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
        return b != 0;
    }

    CEncoding DetectEncoding();
    void WriteEncodingBOM(CEncoding enc);
};