#include "platform.h"
#include <io.h>
#include "str.h"
#include "win.h"
using namespace std;

extern HINSTANCE g_hInst; // needed for dlgOpenFile & dlgSaveFile
map<DWORD, HBRUSH> CBrushCache::mBr;

struct CFindData : public _wfinddata_t {
    intptr_t hFind{};
    CFindData(const string& sFile) {
        hFind = _wfindfirst(utf2w(sFile).c_str(), this);
    }
    bool ok() const { return hFind != -1; }
    ~CFindData() {
        _findclose(hFind);
    }
};

int64_t vGetFileSize(const string& sFile) {
    CFindData fd(sFile);
    return fd.ok() ? fd.size : -1;
}

CTime vGetFileTime(const string& sFile) {
    CFindData fd(sFile);
    return fd.ok() ? CTime(fd.time_write) : CTime(-1);
}

bool vGetFileTimeSize(const string& sFile, CTime& tm, int64_t& size) {
    CFindData fd(sFile);
    if( fd.ok() ) {
        tm = fd.time_write;
        size = fd.size;
    } else {
        tm = {};
        size = 0;
    }
    return fd.ok();
}

string vFileAttrs2Str(const string& sFile) {
    DWORD dw = GetFileAttrs(sFile);
    string s;
    if( dw & FILE_ATTRIBUTE_SYSTEM ) s += 'S';
    if( dw & FILE_ATTRIBUTE_HIDDEN ) s += 'H';
    if( dw & FILE_ATTRIBUTE_ARCHIVE ) s += 'A';
    if( dw & FILE_ATTRIBUTE_READONLY ) s += 'R';
    return s;
}

string GetLastErrorText() {
    char* szMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (char*) &szMsgBuf,
        0, nullptr);
    string s = szMsgBuf;
    LocalFree(szMsgBuf);

    return s;
}

void MsgBox(const char* szMsg) {
    MessageBoxA(nullptr, szMsg, "Information", MB_OK | MB_ICONINFORMATION);
}

void MsgBoxError(const char* szMsg) {
    MessageBoxA(nullptr, szMsg, "Information", MB_OK | MB_ICONSTOP);
}

void MsgBox(const wchar_t* szMsg) {
    MessageBoxW(nullptr, szMsg, L"Information", MB_OK | MB_ICONINFORMATION);
}

void MsgBoxError(const wchar_t* szMsg) {
    MessageBoxW(nullptr, szMsg, L"Information", MB_OK | MB_ICONSTOP);
}

string LoadTextFromResource(const char* szResID) {
    HRSRC hRes = FindResource(nullptr, szResID, "TEXT");
    if( !hRes ) return {};
    HGLOBAL hPtr = LoadResource(nullptr, hRes);
    if( !hPtr ) return {};
    const char* szData = (const char*) LockResource(hPtr);
    if( !szData ) return {};
    DWORD size = SizeofResource(nullptr, hRes);
//    size_t len = strlen(szData);
//    ASSERT( size!=len );
    return {szData, size};
}

bool LoadTextFromFile(const string& szFile, string& sText) {
    sText.clear();
    FILE* f;
    if( !(f = fopen(szFile.c_str(), "rt")) )
        return false;
    char buf[1024];
    while( !feof(f) ) {
        size_t k = fread(buf, 1, sizeof(buf) - 1, f);
        sText.append(buf, k);
    }
    fclose(f);
    return true;
}

bool isAbsolutePath(const string& s) {
    if( s.length() < 3 ) return false;
    if( IsCharAlpha(s[0]) && s[1] == ':' && s[2] == '\\' ) return true;
    if( s[0] == '\\' && s[1] == '\\' ) return true;
    return false;
}

void EnsureFilePath(string& sFile, string sPath) {
    if( !sFile.empty() && (sFile[0] == '-' || sFile[0] == '/') )
        return;
    size_t len = sFile.length();
    if( len > 2 && sFile[0] == '"' && sFile[len - 1] == '"' )
        sFile = sFile.substr(1, len - 2);
    replaceStr(sFile, "/", "\\");
    replaceStr(sFile, "%20", " ");
    string sFilePath = ExtractFilePath(sFile);
    if( !sFilePath.empty() && isAbsolutePath(sFilePath) )
        return;
    if( sPath.empty() )
        sPath = GetCurDir();
    addTailSlash(sPath);
    sFile = sPath + sFile;
}

bool EqualsFilePath(string sFile1, string sFile2) {
    replaceStr(sFile1, "/", "\\");
    replaceStr(sFile2, "/", "\\");
    return equalsTo(sFile1, sFile2);
}

bool CreateProcess(const string& s) {
    STARTUPINFO si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);
    return ::CreateProcess(nullptr, (LPTSTR) s.c_str(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) == TRUE;
}

bool CreateProcessW(const wstring& ws) {
	STARTUPINFOW si = {};
	PROCESS_INFORMATION pi = {};
	si.cb = sizeof(si);
	return ::CreateProcessW(nullptr, (LPWSTR) ws.c_str(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) == TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool dlgOpenFile(HWND hWnd, string& sFile) {
    wstrbuf sBuf(MAX_PATH);
	if( !sFile.empty() )
		sBuf.set(utf2w(sFile));
    const wstring initialDir = utf2w(GetCurDir());
    OPENFILENAMEW ofn {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hInstance = g_hInst;
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = sBuf.buf();
    ofn.nMaxFile = sBuf.size();
    ofn.lpstrInitialDir = initialDir.c_str();
    ofn.Flags = OFN_HIDEREADONLY;
    bool ret = GetOpenFileNameW(&ofn) != 0;
	if( ret )
    	sFile = w2utf(sBuf.str()); // no len
    return ret;
}

bool dlgSaveFile(HWND hWnd, string& sFile) {
    wstrbuf sBuf(MAX_PATH);
    if( !sFile.empty() )
        sBuf.set(utf2w(sFile));
    wstring initialDir = utf2w(GetCurDir());
    OPENFILENAMEW ofn {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hInstance = g_hInst;
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = sBuf.buf();
    ofn.nMaxFile = sBuf.size();
    ofn.lpstrInitialDir = initialDir.c_str();
    ofn.Flags = OFN_HIDEREADONLY;
    bool ret = GetSaveFileNameW(&ofn) != 0;
	if( ret )
    	sFile = w2utf(sBuf.str()); // no len
    return ret;
}

bool dlgChooseColor(HWND hWnd, int& clr) {
    static COLORREF custColors[16];
    CHOOSECOLOR cc = {0};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hWnd;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    cc.lpCustColors = custColors;
    cc.rgbResult = clr;
    if( ChooseColor(&cc) ) {
        clr = cc.rgbResult;
        return true;
    }
    return false;
}

string clr2str(int clr) {
    return Format("#%02X%02X%02X", GetRValue(clr), GetGValue(clr), GetBValue(clr));
}

int str2clr(const string& str) {
    if( startsWith(str, "#") ) {
        int R, G, B;
        if( str.size() == 4 && sscanf(str.c_str(), "#%1X%1X%1X", &R, &G, &B) == 3 )
            return RGB(R, G, B);
        if( str.size() == 7 && sscanf(str.c_str(), "#%02X%02X%02X", &R, &G, &B) == 3 )
            return RGB(R, G, B);
    }
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <Commctrl.h>

BOOL HandleDialogTooltips(NMHDR* pNMHDR, HMENU hMenu) {
//    ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);

    auto* pTTTA = (TOOLTIPTEXTA*) pNMHDR;
    auto* pTTTW = (TOOLTIPTEXTW*) pNMHDR;
    UINT nID = (UINT) pNMHDR->idFrom;

    if( pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
        pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND) ) {
        nID = ::GetDlgCtrlID((HWND) nID); // idFrom is actually the HWND of the tool
    }

    if( nID != 0 ) { // will be zero on a separator
        wstring sTipText = GetMenuTextW(hMenu, nID, MF_BYCOMMAND);
        size_t n = sTipText.find('\n');
        if( n != string::npos )
            sTipText.erase(sTipText.begin(), sTipText.begin() + n + 1);
        n = sTipText.find('\t');
        if( n != string::npos )
            sTipText.erase(sTipText.begin() + n, sTipText.end());
        n = sTipText.find(L"...");
        if( n != string::npos )
            sTipText.erase(sTipText.begin() + n, sTipText.end());

        if( pNMHDR->code == TTN_NEEDTEXTA )
            wcstombs(pTTTA->szText, sTipText.c_str(), sizeof(pTTTA->szText));
        else
            lstrcpynW(pTTTW->szText, sTipText.c_str(), sizeof(pTTTW->szText));

        // bring the tooltip window above other popup windows
        ::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
                       SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER);

        return TRUE;
    }
    return FALSE;
}
