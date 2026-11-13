#include "stdafx.h"
#include "resource.h"
#include "dlgAbout.h"
#include "lang.h"
#include "shared/str.h"
#include "apeVersion.h"

using namespace std;

#define SCINTILLA_VERSION_STR "5.6.6" // from scintilla/win32/ScintRes.rc

void CAboutDlg::onInit() {
    CenterWindow(m_hWnd);
    g_lang.translateDlg(m_hWnd);
    {
        string label;
        label << APE_PRODUCT " v" APE_VERSION_STR "\n"
              << "Scintilla " SCINTILLA_VERSION_STR "\n\n"
              << APE_COPYRIGHT " " APE_COMPANY "\n\n"
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