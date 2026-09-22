#include "stdafx.h"
#include "resource.h"
#include "dlgAbout.h"
#include "lang.h"
#include "shared/str.h"
#include "versionInfo.h"

using namespace std;

// Version of the Scintilla component vendored in src\scintilla (Scintilla has no
// version macro of its own, so this is updated by hand when it is upgraded)
#define SCINTILLA_VERSION_STR "5.6.6"
// todo: ScintRes.rc #define VERSION_SCINTILLA "5.6.6"

void CAboutDlg::onInit() {
    CenterWindow(m_hWnd);
    g_lang.translateDlg(m_hWnd);
    {
        CVersionInfo vi;
        string version = vi.getFileVersion();
        replaceStr(version, ", ", ".");
        string label;
        label << vi.getFileDescription() << ' '
              << 'v' << version << "\n"
              << "Scintilla " SCINTILLA_VERSION_STR "\n\n"
              << vi.getLegalCopyright() << ' '
              << vi.getCompanyName() << "\n\n"
              << "Built on " << __DATE__ << ' ' << __TIME__;
        label << "\nwith "
#ifdef CLION
            << "CLion";
#else
            << "MSVC";
#endif
        label << ' '
#ifdef _M_X64
            << "x64";
#else
            << "x86";
#endif
        label << ' '
#ifdef NDEBUG
            << "Release";
#else
            << "Debug";
#endif
        CStatic().Attach(m_hWnd, ID_ABOUT_LABEL).SetText(label);
    }
}

void CAboutDlg::onCommand(int cmd, int) {
    if( cmd == IDOK || cmd == IDCANCEL )
        EndDialog(m_hWnd, IDOK);
}