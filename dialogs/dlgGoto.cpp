#include "stdafx.h"
#include "resource.h"
#include "dlgGoto.h"
#include "lang.h"

void CGotoDlg::onInit() {
    CenterWindow(m_hWnd);
    g_lang.translateDlg(m_hWnd);

    edGoto.Attach(m_hWnd, IDC_GOTO_NUMBER);
    stcCur.Attach(m_hWnd, IDC_GOTO_CUR);
    stcLast.Attach(m_hWnd, IDC_GOTO_LAST);
    lblCur.Attach(m_hWnd, IDC_GOTO_CUR_LBL);
    lblLast.Attach(m_hWnd, IDC_GOTO_LAST_LBL);

    edGoto.SetText("1");

    int cmd = bGotoLine ? IDC_GOTO_LINE : IDC_GOTO_POS;
    CheckDlgButton(m_hWnd, cmd, BST_CHECKED);
    onCommand(cmd,0);
}

void CGotoDlg::onCommand(int cmd, int) {
    switch( cmd ) {
        case IDC_GOTO_POS :
            lblCur.SetText(tr(L"Current position is:"));
            lblLast.SetText(tr(L"Last position is:"));
            stcCur.SetText(itoa(iCurPos));
            stcLast.SetText(itoa(iLastPos));
            break;
        case IDC_GOTO_LINE :
            lblCur.SetText(tr(L"Current line is:"));
            lblLast.SetText(tr(L"Last line is:"));
            stcCur.SetText(itoa(iCurLine + 1));
            stcLast.SetText(itoa(iLastLine));
            break;

        case IDOK:
        case IDCANCEL:
            if( cmd == IDOK ) {
                iGoto = stoi(edGoto.GetText());
                bGotoLine = IsDlgButtonChecked(m_hWnd, IDC_GOTO_LINE) != 0;
                if( bGotoLine )
                    iGoto--;
            }
            EndDialog(m_hWnd, cmd);
            break;
    }
}