#pragma once

#include "sci.h"
#include "shared/win.h"

class CEditor;

struct CREOpt {
    const char* szTitle;
    const char* szText;
};

class CFindDlg : public CDialog {
    CComboBox cmbSearch, cmbReplace, cmbFile, cmbPath;
    CCheckBox chkReplace, chkInFiles;
    CCheckBox chkWholeWord, chkMatchCase;
    CCheckBox chkSubFolders, chkNewPane, chkCreateBak;
    CButton btnSearchOpt, btnReplaceOpt, btnBrowse;
    CCheckBox rbtnUp, rbtnDown;
    CCheckBox rbtnWhole, rbtnSelection;
    CButton btnSearch, btnSearchAll, btnReplace, btnReplaceAll;
    CComboBox cmbMode;
    bool bFind;
    CEditor* pEditor;

    bool isModeNormal() { return cmbMode.GetCurSel() == 0; }
    bool isModeExt() { return cmbMode.GetCurSel() == 1; }
    bool isModeSciRE() { return cmbMode.GetCurSel() == 2; }
    bool isModeBoostRE() { return cmbMode.GetCurSel() == 3; }
    bool isModeRE() { return cmbMode.GetCurSel() >= 2; }
public:
    explicit CFindDlg(CEditor* pEditor_) : bFind(false), pEditor(pEditor_) {
        m_IDD=IDD_FIND;
    }
    bool isDlgMsg(MSG& msg);

    void Find(bool bReplace, bool bInFiles);
    void FindNext(bool bNext);
    void ClearIndicators();
private:
    void onInit() override;
    void onDestroy() override;
    void onCommand(int cmd, int code) override;

    void UpdateControls(bool bPrepare = false);

    int getFindFlags();
    string getFindStr();
    string getReplaceStr();

    bool onFind();
    void onReplace();
    void onFindAll();
    void onReplaceAll();

    static void popupOptRE(CREOpt* opts, HWND hDlg, HWND hBtn, HWND hEdit);
};

void replaceSciRegExp(CSciWrapper* pSci, const string& sFind, const string& sReplace);
void replaceNormal(CSciWrapper* pSci, const string& sFind, const string& sReplace);