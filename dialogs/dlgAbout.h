#pragma once

#include "shared/win.h"

class CAboutDlg : public CDialog {
public:
    CAboutDlg() { m_IDD=IDD_ABOUT; }
private:
    void onInit() override;
    void onCommand(int cmd, int code) override;
};