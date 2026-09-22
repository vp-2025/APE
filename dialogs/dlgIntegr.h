#pragma once

#include "shared/win.h"

class CIntegrationDlg : public CDialog {
    CButton btnReg, btnUnreg;
    CButton btnSet, btnRemove;
    bool m_bSkip{};
public:
    CIntegrationDlg() { m_IDD=IDD_INTEGR; }
private:
    void onInit() override;
    void onCommand(int cmd, int code) override;
    void UpdateBtnState();
};

bool isNotepadReplacement();
void setNotepadReplacement(bool b, bool bShowError = true);
