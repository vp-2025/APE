#pragma once

#include "shared/win.h"

class COptionsDlg : public CDialog {
    CCheckBox chkCheckExtModify,
        chkOneInstance,
        chkAutoIndent,
        chkSaveOnDeactivate,
        chkStripTrailSpaces,
        chkTabsCloseBtn,
        chkTabsDragNDrop;
    CEdit edTabSize;
public:
    COptionsDlg() { m_IDD = IDD_OPTIONS; }
private:
    void onInit() override;
    void onCommand(int cmd, int code) override;
    void ACX(bool bSave);
};