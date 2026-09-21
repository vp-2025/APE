#include "stdafx.h"
#include "resource.h"
#include "dlgOptions.h"
#include "lang.h"
#include "options.h"

void COptionsDlg::ACX(bool bSave) {
    acx(g_options.bCheckExternalModify, chkCheckExtModify, bSave);
    acx(g_options.bOneInstanse, chkOneInstance, bSave);
    acx(g_options.bAutoIndent, chkAutoIndent, bSave);
    acx(g_options.bSaveOnDeactivate, chkSaveOnDeactivate, bSave);
    acx(g_options.bStripTrailingSpaces, chkStripTrailSpaces, bSave);
    acx(g_options.iTabSize, edTabSize, bSave);
    acx(g_options.bTabsCloseBtn, chkTabsCloseBtn, bSave);
    acx(g_options.bTabsDragNDrop, chkTabsDragNDrop, bSave);
}

void COptionsDlg::onInit() {
    chkCheckExtModify.Attach(m_hWnd, IDC_OPT_CHK_EXT_MODIFY);
    chkOneInstance.Attach(m_hWnd, IDC_OPT_CHK_ONE_INSTANCE);
    chkAutoIndent.Attach(m_hWnd, IDC_OPT_CHK_AUTOINDENT);
    chkSaveOnDeactivate.Attach(m_hWnd, IDC_OPT_CHK_SAVEONDEACTIVATE);
    chkStripTrailSpaces.Attach(m_hWnd, IDC_OPT_CHK_STRIPTRAILSPACE);
    edTabSize.Attach(m_hWnd, IDC_OPT_TABSIZE);
    chkTabsCloseBtn.Attach(m_hWnd, IDC_OPT_TABS_CLOSEBTN);
    chkTabsDragNDrop.Attach(m_hWnd, IDC_OPT_TABS_DRAGNDROP);

    CenterWindow(m_hWnd);
    g_lang.translateDlg(m_hWnd);

    ACX(false);
}

void COptionsDlg::onCommand(int cmd, int code) {
    switch( cmd ) {
        case IDOK:
            ACX(true);
            g_options.Save();
        case IDCANCEL:
            EndDialog(m_hWnd, cmd);
        default:
            return;
    }
}