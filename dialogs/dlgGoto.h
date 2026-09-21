#pragma once

#include "shared/win.h"

class CGotoDlg : public CDialog {
    CEdit edGoto;
    CStatic lblCur, lblLast, stcCur, stcLast;
public:
    int iCurPos, iLastPos;
    int iCurLine, iLastLine;
    int iGoto;
    bool bGotoLine;

    explicit CGotoDlg() {
        m_IDD = IDD_GOTO;
        iCurPos = iLastPos = iCurLine = iLastLine = iGoto = 0;
        bGotoLine = false;
    }
private:
    void onInit() override;
    void onCommand(int cmd, int code) override;
};