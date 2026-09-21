#pragma once

#include "shared/win.h"
#include "sci.h"

class CSortDlg : public CDialog {
    CCheckBox chkAsc, chkIgnoreCase, chkTrim;
    CSciWrapper& sci;
public:
    explicit CSortDlg(CSciWrapper& _sci) : sci(_sci) { m_IDD = IDD_SORT; }
private:
    void onInit() override;
    void onCommand(int cmd, int code) override;
};

void sortLines(CSciWrapper& sci, bool bAsc, bool bIgnoreCase, bool bTrim);