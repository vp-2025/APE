#include "stdafx.h"
#include "resource.h"
#include "dlgIntegr.h"
#include "lang.h"
#include "shared/str.h"
#include "shared/platform.h"

using namespace std;

///////////////////////////////////////////////////////////
// Context Menu 

typedef HRESULT ( WINAPI* PFN_DLLFunction )();

class CContextMenu {
    HMODULE g_hDLL;
    PFN_DLLFunction g_hRegisterServer, g_hUnregisterServer, g_hIsRegisteredServer;
public:
    CContextMenu() {
        g_hDLL = 0;
        g_hRegisterServer = g_hUnregisterServer = g_hIsRegisteredServer = 0;
        string sDLL = GetExePath() + "apecm.dll";
        g_hDLL = LoadLibrary(sDLL.c_str());
        if( !g_hDLL ) return;
        g_hRegisterServer = (PFN_DLLFunction) GetProcAddress(g_hDLL, "DllRegisterServer");
        g_hUnregisterServer = (PFN_DLLFunction) GetProcAddress(g_hDLL, "DllUnregisterServer");
        g_hIsRegisteredServer = (PFN_DLLFunction) GetProcAddress(g_hDLL, "DllIsRegisteredServer");
    }
    bool isContextMenu() {
        if( !g_hDLL || !g_hIsRegisteredServer )
            return false;
        return g_hIsRegisteredServer() == S_OK;
    }
    void setContextMenu(bool b) {
        if( !g_hDLL || !g_hRegisterServer || !g_hUnregisterServer )
            return;
        bool ok;
        if( b )
            ok = g_hRegisterServer() == S_OK;
        else
            ok = g_hUnregisterServer() == S_OK;
        if( !ok )
            return MsgBoxError("Error in SetContextMenu");
        string sDLL = GetExePath() + "apecm64.dll";
        if( IsFileExists(sDLL) )
            CreateProcess(string("regsvr32 /s ") + (b ? "" : "/u ") + sDLL);
    }
};

CContextMenu contextMenu;

///////////////////////////////////////////////////////////
// Notepad Replacement

const char* szKey = R"(SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\notepad.exe)";
const char* szVal = "Debugger";

bool isNotepadReplacement() {
    CRegistry reg;
    if( reg.Open(HKEY_LOCAL_MACHINE, szKey) ) {
        string sExe;
        sExe << "\"" << GetExeFileName() << "\" -d";
        return sExe == reg.ReadStr(szVal);
    }
    return false;
}

void setNotepadReplacement(bool b, bool bShowError) {
    CRegistry reg;
    if( b ) {
        string sExe;
        sExe << "\"" << GetExeFileName() << "\" -d";

        if( reg.Create(HKEY_LOCAL_MACHINE, szKey) ) {
            reg.WriteStr(szVal, sExe);
            return;
        }
    } else {
        if( CRegistry::Delete(HKEY_LOCAL_MACHINE, szKey) )
            return;
    }
    if( bShowError )
        MsgBoxError("Error in SetNotepadReplacement");
}

///////////////////////////////////////////////////////////

void CIntegrationDlg::onInit() {
    CenterWindow(m_hWnd);
    g_lang.translateDlg(m_hWnd);

    btnReg.Attach(m_hWnd, IDC_BTN_CM_REG);
    btnUnreg.Attach(m_hWnd, IDC_BTN_CM_UNREG);
    btnSet.Attach(m_hWnd, IDC_BTN_NOTE_SET);
    btnRemove.Attach(m_hWnd, IDC_BTN_NOTE_REMOVE);

    m_bSkip = false;
    {
        CRegistry reg;
        const char* szKeyA = "CLSID";
        bool bA = !reg.Open(HKEY_CLASSES_ROOT, szKeyA);
        int retA = reg.ret();
        reg.Close();
        const char* szKeyB = "*\\shellex\\ContextMenuHandlers";
        bool bB = !reg.Open(HKEY_CLASSES_ROOT, szKeyB);
        int retB = reg.ret();
        reg.Close();
        const char* szKeyC = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options";
        bool bC = !reg.Open(HKEY_LOCAL_MACHINE, szKeyC);
        int retC = reg.ret();
        if( bA || bB || bC ) {
            btnReg.Enable(false);
            btnUnreg.Enable(false);
            btnSet.Enable(false);
            btnRemove.Enable(false);
            m_bSkip = true;
            MsgBoxError(
                "For this functionality under Vista or Windows 7 you need to turn off 'User Acount Control' & reboot");

            string s;
            s << "A=" << retA << " B=" << retB << " C=" << retC;
            MsgBoxError(s);

            OSVERSIONINFO vi = {0};
            vi.dwOSVersionInfoSize = sizeof(vi);
            GetVersionEx(&vi);
            s.clear();
            s << "Windows: " << vi.dwMajorVersion << ' ' << vi.dwMinorVersion << ' ' << vi.dwBuildNumber << ' '
              << vi.szCSDVersion;
            MsgBoxError(s);
        }
    }
    UpdateBtnState();
}

void CIntegrationDlg::onCommand(int cmd, int) {
    switch( cmd ) {
        case IDOK:
        case IDCANCEL:
            EndDialog(m_hWnd, cmd);
            break;
        case IDC_BTN_CM_REG:
            contextMenu.setContextMenu(true);
            break;
        case IDC_BTN_CM_UNREG:
            contextMenu.setContextMenu(false);
            break;
        case IDC_BTN_NOTE_SET:
            setNotepadReplacement(true);
            break;
        case IDC_BTN_NOTE_REMOVE:
            setNotepadReplacement(false);
            break;
    }
    UpdateBtnState();
}

void CIntegrationDlg::UpdateBtnState() {
    bool b;
    if( !m_bSkip ) {
        b = contextMenu.isContextMenu();
        btnReg.Enable(!b);
        btnUnreg.Enable(b);

        b = isNotepadReplacement();
        btnSet.Enable(!b);
        btnRemove.Enable(b);
    }
}
