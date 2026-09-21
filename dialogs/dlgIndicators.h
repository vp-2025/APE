#pragma once

#include "sci.h"
#include "shared/platform.h"
#include "shared/win.h"
#include "lexers.h"

struct CIndiData {
    CIndicator* pIndi;
    int color, alpha, outlineAlpha, style;
    CStatic stcLabel;
    CComboBox cmbStyle;
    CButton btnColor;
    CComboBox cmbAlpha, cmbOutlineAlpha;
    CSciWrapper* pSci;
    int id;
private:
    CIndiData();
public:
    CIndiData(CIndicator& indi, CSciWrapper* pSci_, int id_) {
        pIndi = &indi;
        pSci = pSci_;
        id = id_;
        color = indi.color;
        alpha = indi.alpha;
        outlineAlpha = indi.outlineAlpha;
        style = indi.style;
    }
    void store() const {
        if( !pIndi ) return;
        pIndi->style = style;
        pIndi->color = color;
        pIndi->alpha = alpha;
        pIndi->outlineAlpha = outlineAlpha;
    }

    void apply2Sample() const {
        if( pSci )
            pSci->Indicator(id).Fore(color).Alpha(alpha * 2.55).OutlineAlpha(outlineAlpha * 2.55).Style(style).Under(
                true);
    }

    void setData() {
        cmbStyle.SetupComboEx(style);
        btnColor.SetText(clr2str(color));
        cmbAlpha.SetupComboEx(alpha);
        cmbOutlineAlpha.SetupComboEx(outlineAlpha);
        updateControls();
    }
    void getData() {
        style = cmbStyle.GetComboEx();
        color = str2clr(btnColor.GetText());
        alpha = cmbAlpha.GetComboEx();
        outlineAlpha = cmbOutlineAlpha.GetComboEx();
        updateControls();
    }
    void updateControls() {
        bool b = style == INDIC_ROUNDBOX || style == INDIC_STRAIGHTBOX;
        cmbAlpha.Enable(b);
        cmbOutlineAlpha.Enable(b);
    }
};

class CIndicatorsDlg : public CDialog {
    vector<CIndiData> vIndi;
    CSciWrapper sci;
public:
    CIndicatorsDlg() { m_IDD = IDD_INDICATORS; }
private:
    void onInit() override;
    void onDestroy() override { CBrushCache::clear(); }
    void onCommand(int cmd, int code) override;
    BOOL onDrawClrBtn(DRAWITEMSTRUCT* pDIS) override;
};