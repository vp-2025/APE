#include "stdafx.h"
#include "resource.h"
#include "dlgIntegr.h"
#include "lang.h"
#include "shared/str.h"
#include "shared/platform.h"

using namespace std;

const char* szApeKey = R"(*\shell\ape)";
const char* szApeCmdKey = R"(*\shell\ape\command)";
const char* szApeMenuText = "Edit with &APE++";

string getApeMenuCommand() {
    return '"' + GetExeFileName() + R"(" "%1")";
}

static bool ensureKey(HKEY hRoot, const char* szPath) {
    CRegistry reg;
    return reg.Create(hRoot, szPath);
}

static bool setKey(HKEY hRoot, const char* szPath, const char* szKey, const char* szValue) {
    CRegistry reg;
    if( !reg.Create(hRoot, szPath) )
        return false;
    reg.WriteStr(szKey, szValue);
    return true;
}

bool isContextMenu() {
    CRegistry regShell, regCmd;
    if( !regShell.Open(HKEY_CLASSES_ROOT, szApeKey) || regShell.ReadStr("") != szApeMenuText )
        return false;
    if( !regCmd.Open(HKEY_CLASSES_ROOT, szApeCmdKey) )
        return false;
    return regCmd.ReadStr("") == getApeMenuCommand();
}

void setContextMenu(bool b) {
    if( b ) {
        if( !ensureKey(HKEY_CLASSES_ROOT, "*\\shell")
                || !setKey(HKEY_CLASSES_ROOT, szApeKey, "", szApeMenuText)
                || !setKey(HKEY_CLASSES_ROOT, szApeKey, "Icon", (GetExeFileName()+",0").c_str())
                || !setKey(HKEY_CLASSES_ROOT, szApeCmdKey, "", getApeMenuCommand().c_str()) )
            return MsgBoxError("Error in setContextMenu");
    } else {
        if( !CRegistry::Delete(HKEY_CLASSES_ROOT, szApeCmdKey)
            || !CRegistry::Delete(HKEY_CLASSES_ROOT, szApeKey) )
            return MsgBoxError("Error in delContextMenu");
    }
}

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
            setContextMenu(true);
            break;
        case IDC_BTN_CM_UNREG:
            setContextMenu(false);
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
        b = isContextMenu();
        btnReg.Enable(!b);
        btnUnreg.Enable(b);

        b = isNotepadReplacement();
        btnSet.Enable(!b);
        btnRemove.Enable(b);
    }
}
