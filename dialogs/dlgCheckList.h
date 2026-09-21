#pragma once

#include "shared/win.h"
#include "tabs.h"
#include "tabPage.h"

class CCheckListDlg : public CDialog {
    CListCtrl list;
    bool bReload;
    const vector<CTabPage>& m_vTabs;
    const CShellImageList& m_ilTabs;
    set<string> stChanged;
public:
    enum {
        modeSaveChanges,    // onCanClose
        modeReloadChanges    // CheckExternalModify
    };

    CCheckListDlg(const vector<CTabPage>& vTabs, const CShellImageList& ilTabs, int mode)
		: m_vTabs(vTabs), m_ilTabs(ilTabs), bReload(mode == modeReloadChanges) {
        m_IDD = IDD_CHECK_LIST;
    }

    bool isChanged(const string& sFile) const { return stChanged.find(sFile) != stChanged.end(); }
private:
    void onInit() override;
    void onCommand(int cmd, int code) override;
};