#include "cfile.h"
#include "str.h"
#include "platform.h" // IsFileExists

using namespace std;

const char bomUtf8[] = {(char) 0xEF, (char) 0xBB, (char) 0xBF, 0};
const char bomUtf16BE[] = {(char) 0xFE, (char) 0xFF, 0};
const char bomUtf16LE[] = {(char) 0xFF, (char) 0xFE, 0};

bool CFile::Open(const string& sFile, bool bRead) {
    if( IsOpen() ) return false;
    if( bRead )
        hFile = CreateFileW(utf2w(sFile).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    else
        hFile = CreateFileW(utf2w(sFile).c_str(), GENERIC_WRITE, 0, nullptr,
                           IsFileExists(sFile) ? TRUNCATE_EXISTING : CREATE_ALWAYS, 0, nullptr);
    return IsOpen();
}

CEncoding CFile::DetectEncoding() {
    char bufT[128];
    size_t len = Read(bufT, sizeof(bufT) - 1);
    string sBuf(bufT, len);
    size_t skip = 0;
    CEncoding enc = encAnsi;
    if( startsWith(sBuf, bomUtf8) ) {
        enc = encUtf8;
        skip = strlen(bomUtf8);
    } else if( startsWith(sBuf, bomUtf16BE) ) {
        enc = encUtf16BE;
        skip = strlen(bomUtf16BE);
    } else if( startsWith(sBuf, bomUtf16LE) ) {
        enc = encUtf16LE;
        skip = strlen(bomUtf16LE);
    } else {
        toLower(sBuf);
        if( contains(sBuf, "coding") ) {
			if( contains(sBuf, "utf-8") )
				enc = encUtf8NoBOM;
			else if( contains(sBuf, "latin1") || contains(sBuf, "latin-1") || contains(sBuf, "8859-1") )
				enc = encLatin1;
			else if( contains(sBuf, "latin2") || contains(sBuf, "latin-2") || contains(sBuf, "8859-2") )
				enc = encLatin2;
			else if( contains(sBuf, "latin3") || contains(sBuf, "latin-3") || contains(sBuf, "8859-3") )
				enc = encLatin3;
			else if( contains(sBuf, "windows-1251") )
				enc = encAnsiCyr;
        }
    }
    SetFilePointer(hFile, (long) skip, nullptr, FILE_BEGIN);
    // No BOM and no encoding hint: the default encoding is UTF-8 (without BOM),
    // as it is for new documents. Only text that cannot be UTF-8 at all is
    // opened as ANSI, so legacy ANSI/Latin files stay readable.
    if( enc==encAnsi && IsValidUtf8() )
        enc = encUtf8NoBOM;
    return enc;
}

// Is the rest of the file valid UTF-8? Used to tell a BOM-less UTF-8 file from a
// legacy 8-bit one. The file pointer is left where it was.
bool CFile::IsValidUtf8() {
    DWORD pos0 = SetFilePointer(hFile, 0, nullptr, FILE_CURRENT);
    char buf[64 * 1024];
    int nNeed = 0;              // continuation bytes still expected
    bool bValid = true;
    while( bValid ) {
        DWORD len = Read(buf, sizeof(buf));
        if( !len ) break;
        for( DWORD i = 0; i < len; i++ ) {
            unsigned char ch = (unsigned char) buf[i];
            if( nNeed ) {
                if( ch < 0x80 || ch > 0xBF ) {  // not a continuation byte
                    bValid = false;
                    break;
                }
                nNeed--;
            } else if( ch < 0x80 ) {
                // ascii
            } else if( ch >= 0xC2 && ch <= 0xDF ) {
                nNeed = 1;
            } else if( ch >= 0xE0 && ch <= 0xEF ) {
                nNeed = 2;
            } else if( ch >= 0xF0 && ch <= 0xF4 ) {
                nNeed = 3;
            } else {                            // 0x80..0xC1, 0xF5..0xFF
                bValid = false;
                break;
            }
        }
    }
    if( bValid && nNeed )
        bValid = false;                     // the file ends inside a character
    SetFilePointer(hFile, (long) pos0, nullptr, FILE_BEGIN);
    return bValid;
}

void CFile::WriteEncodingBOM(CEncoding enc) {
    const char* BOM;
    switch( enc ) {
        case encUtf8:
            BOM = bomUtf8;
            break;
        case encUtf16BE:
            BOM = bomUtf16BE;
            break;
        case encUtf16LE:
            BOM = bomUtf16LE;
            break;
        default:
            BOM = nullptr;
    }
    if( BOM )
        Write((void*) BOM, strlen(BOM));
}

