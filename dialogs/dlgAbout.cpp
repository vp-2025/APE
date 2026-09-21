#include "stdafx.h"
#include "resource.h"
#include "dlgAbout.h"
#include "lang.h"
#include "shared/str.h"
#include "versionInfo.h"

using namespace std;

void CAboutDlg::onInit() {
    CenterWindow(m_hWnd);
    g_lang.translateDlg(m_hWnd);
    {
        CVersionInfo vi;
        string version = vi.getFileVersion();
        replaceStr(version, ", ", ".");
        string label;
        label << vi.getFileDescription() << ' '
              << 'v' << version << "\n\n"
              << vi.getLegalCopyright() << ' '
              << vi.getCompanyName() << "\n\n"
              << "Built on " << __DATE__ << ' ' << __TIME__ << " with "
#ifdef CLION
              << "CLion";
#else
        << "VS";
#endif
        CStatic().Attach(m_hWnd, ID_ABOUT_LABEL).SetText(label);
    }
}

void CAboutDlg::onCommand(int cmd, int) {
    if( cmd == IDOK || cmd == IDCANCEL )
        EndDialog(m_hWnd, IDOK);
}