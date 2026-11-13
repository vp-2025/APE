#pragma once

#include "sci.h"
#include "shared/win.h"

class CEditor;
class CFindResults;
struct CFindParams;

struct CREOpt {
    const char* szTitle;
    const char* szText;
    int modes;         // 0: all the modes, else the bits 1 << m_mode
};

class CFindDlg : public CDialog {
    CComboBox cmbSearch, cmbReplace, cmbFile, cmbPath;
    CCheckBox chkReplace, chkInFiles;
    CCheckBox chkWholeWord, chkMatchCase;
    CCheckBox chkSubFolders, chkNewTab, chkCreateBak;
    CButton btnSearchOpt, btnReplaceOpt, btnBrowse;
    CCheckBox rbtnUp, rbtnDown;
    CCheckBox rbtnWhole, rbtnSelection;
    CButton btnSearch, btnSearchAll, btnReplace, btnReplaceAll;
    CCheckBox btnMode; // pressed: not Normal; the modes are in its popup
    int m_mode{};      // 0 Normal, 1 Extended, 2 Scintilla regex, 3 std::regex
    HWND m_hTip{};     // tooltips of Cc, W, mode
    bool bFind;
    CEditor* pEditor;
    CFindResults* m_pRerun{}; // the results tab of Rerun: the next results go to it

    bool isModeNormal() { return m_mode == 0; }
    bool isModeExt() { return m_mode == 1; }
    bool isModeSciRE() { return m_mode == 2; }
    bool isModeStdRE() { return m_mode == 3; }
    bool isModeRE() { return m_mode >= 2; }
    void setMode(int mode); // + the button: "*." / "\t", pressed, tooltip
    void popupMode();
public:
    explicit CFindDlg(CEditor* pEditor_) : bFind(false), pEditor(pEditor_) {
        m_IDD=IDD_FIND;
    }
    bool isDlgMsg(MSG& msg);

    void Find(bool bReplace, bool bInFiles);
    // the dialog with the search of these results; the next Find All / Replace All / Find in Files fills them
    void Rerun(CFindResults* pResults);
    void onResultsClosed(CFindResults* pResults) { if( m_pRerun == pResults ) m_pRerun = nullptr; }
    void FindNext(bool bNext);
    void ClearIndicators();
private:
    void onInit() override;
    void onDestroy() override;
    void onCommand(int cmd, int code) override;

    void UpdateControls(bool bPrepare = false);

    int getFindFlags();
    // options of the search string, kept in the history of cmbSearch: match case, whole word, mode
    LPARAM getOptions();
    void setOptions(LPARAM opts);
    string getFindStr();
    string getFindStr(const wstring& ws, bool bAnsi); // for a file with this encoding
    string getReplaceStr();
    CFindParams getParams();
    bool resultsInNewTab(); // also selects the tab of Rerun

    bool onFind();
    void onReplace();
    void onFindAll();
    void onFindInFiles();
    void closeToResults(); // the results are filled: the dialog is hidden, the focus goes to the results
    void onReplaceAll();

    static void popupOptRE(CREOpt* opts, int mode, HWND hDlg, HWND hBtn, HWND hEdit);
};

void replaceRE(CSciWrapper* pSci, const string& sFind, const string& sReplace);
void replaceNormal(CSciWrapper* pSci, const string& sFind, const string& sReplace);