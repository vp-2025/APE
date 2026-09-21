#pragma once

#include "sci.h"
#include "lexerStyles.h"
#include "shared/win.h"

class CColorsDlg : public CDialog {
    CTreeCtrl tree;
    CCheckBox chkBold, chkItalic, chkUnder, chkEOL;
    CCheckBox chkFont, chkSize, chkFore, chkBack;
    CEdit edSize;
    CComboBox cmbFont;
    CButton btnFore, btnBack;
    CSciWrapper sci;
    bool m_in;
    CLexerStyle* m_pStyles;
public:
    explicit CColorsDlg(CLexerStyle* pStyles) { m_in = false; m_IDD=IDD_COLORS; m_pStyles=pStyles; }
    void addFont(const char* sz) { cmbFont.AddString(sz); }
private:
    void onInit() override;
    void onDestroy() override { CBrushCache::clear(); }
    void onCommand(int cmd, int code) override;
    void onNotify(LPARAM lParam) override;
    BOOL onDrawClrBtn(DRAWITEMSTRUCT* pDIS) override;

    void UpdateControls();

    void fillStyles(CLexerStyle* pStyles, const char* szName);
    void storeStyles(HTREEITEM hItem);
    void onDeleteItem(HTREEITEM hItem);

    void doDlgColorButton(CButton& btn);
    void setData(HTREEITEM hItem);
    void getData(HTREEITEM hItem);
    void apply2Sample(HTREEITEM hItem = nullptr);
};