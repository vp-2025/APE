#include "stdafx.h"
#include "shared/platform.h"
#include "resource.h"
#include "dlgColors.h"
#include "lang.h"

struct CItemData : public CLexerStyle {
    CLexerStyle* pStyle; // object itself is CLexerStyle, and pStyle for original data

    explicit CItemData(CLexerStyle& stl) {
        pStyle = &stl;
        iMask = stl.iMask;
        iSize = stl.iSize;
        sFont = stl.sFont;
        clrFore = stl.clrFore;
        clrBack = stl.clrBack;
    }
    void store() const {
        if( !pStyle ) return;
        pStyle->iMask = iMask;
        pStyle->iSize = iSize;
        pStyle->sFont = sFont;
        pStyle->clrFore = clrFore;
        pStyle->clrBack = clrBack;
    }
};

int CALLBACK EnumFontProc( const LOGFONT FAR* lf, const TEXTMETRIC FAR* , DWORD , LPARAM lParam) {
    ((CColorsDlg*) lParam)->addFont(lf->lfFaceName);
    return 1;
}

void CColorsDlg::fillStyles(CLexerStyle* pStyles, const char* szName) {
    HTREEITEM hGroup = tree.InsertItem(nullptr, szName, new CItemData(pStyles[0]));
    map<int, HTREEITEM> m;
    for( int i = 1; pStyles[i].szName; i++ ) {
        HTREEITEM hP = pStyles[i].parentId ? m[pStyles[i].parentId] : hGroup;
        m[pStyles[i].id] = tree.InsertItem(hP, pStyles[i].szName, new CItemData(pStyles[i]));
    }
    if( pStyles == m_pStyles ) {
        tree.SelectItem(hGroup);
        tree.Expand(hGroup);
    }
}

void CColorsDlg::storeStyles(HTREEITEM hItem) {
    for( ; hItem; hItem = tree.GetNext(hItem) ) {
        ((CItemData*) tree.GetItemData(hItem))->store();
        storeStyles(tree.GetChild(hItem));
    }
}

void CColorsDlg::onDeleteItem(HTREEITEM hItem) {
    if( !hItem ) return;
    delete (CItemData*) tree.GetItemData(hItem);
}

void CColorsDlg::onInit() {
    tree.Attach(m_hWnd, IDC_EDITORHL_TREE);
    chkBold.Attach(m_hWnd, IDC_EDITORHL_BOLD);
    chkItalic.Attach(m_hWnd, IDC_EDITORHL_ITALIC);
    chkUnder.Attach(m_hWnd, IDC_EDITORHL_UNDERLINE);
    chkEOL.Attach(m_hWnd, IDC_EDITORHL_EOL);

    chkSize.Attach(m_hWnd, IDC_EDITORHL_SIZE_C);
    chkFont.Attach(m_hWnd, IDC_EDITORHL_FONT_C);
    chkFore.Attach(m_hWnd, IDC_EDITORHL_FORE_C);
    chkBack.Attach(m_hWnd, IDC_EDITORHL_BACK_C);

    edSize.Attach(m_hWnd, IDC_EDITORHL_SIZE);
    cmbFont.Attach(m_hWnd, IDC_EDITORHL_FONT);
    btnFore.Attach(m_hWnd, IDC_EDITORHL_CLRFORE);
    btnBack.Attach(m_hWnd, IDC_EDITORHL_CLRBACK);

    ::SendMessage(GetDlgItem(m_hWnd, IDC_EDITORHL_SPINSIZE), UDM_SETRANGE, 0, MAKELPARAM(100, 1));

    CenterWindow(m_hWnd);
    g_lang.translateDlg(m_hWnd);

    HDC hdc = GetDC(m_hWnd);
    EnumFontFamilies(hdc, nullptr, EnumFontProc, (LPARAM) this);
    ReleaseDC(m_hWnd, hdc);

    CStatic stcSample;
    stcSample.Attach(m_hWnd, IDC_EDITORHL_SAMPLE);
    CRect r = stcSample.GetRect();

    sci.Create(stcSample.hWnd(), 0, 0, r.width(), r.height());
    ::ShowWindow(sci.hWnd(), SW_SHOW);
    sci.ViewNums(false);
    sci.ViewFold(false);
    sci.SetHScrollBar(false);
    sci.SetVScrollbar(false);
    sci.AddText("Sample Text");
    sci.SetReadOnly(true);
    sci.SetCarretLineVisible(false);

    for( int i = 0; g_styles[i].szName; i++ )
        fillStyles(g_styles[i].styles, g_styles[i].szName);
}

void CColorsDlg::doDlgColorButton(CButton& btn) {
    int clr = str2clr(btn.GetText());
    if( dlgChooseColor(m_hWnd, clr) ) {
        btn.SetText(clr2str(clr));
        apply2Sample();
    }
}

void CColorsDlg::onCommand(int cmd, int code) {
    switch( cmd ) {
        case IDC_EDITORHL_CLRFORE:
            doDlgColorButton(btnFore);
            break;
        case IDC_EDITORHL_CLRBACK:
            doDlgColorButton(btnBack);
            break;

        case IDOK:
            storeStyles(tree.GetRoot());
            WriteLexerStyles();
        case IDCANCEL:
            EndDialog(m_hWnd, cmd);
            break;

        default:
            UpdateControls();
            if( !m_in )
                apply2Sample();
    }
}

void CColorsDlg::apply2Sample(HTREEITEM hItem) {
    if( !hItem ) {
        hItem = tree.GetSelection();
        getData(hItem);
    }
    sci.StyleResetDefault();
    sci.StylesClear();
    vector<CItemData*> vHier;
    for( ; hItem; hItem = tree.GetParent(hItem) )
        vHier.push_back((CItemData*) tree.GetItemData(hItem));
    vHier.push_back((CItemData*) tree.GetItemData(tree.GetRoot()));
    for( int i = (int)vHier.size() - 1; i >= 0; i-- ) {
        CItemData* pData = vHier[i];

        CSciStyle stl = sci.Style(0);
        if( pData->isBold )
            stl.Bold(pData->bBold);
        if( pData->isItalic )
            stl.Italic(pData->bItalic);
        if( pData->isUnderline )
            stl.Underline(pData->bUnderline);
        if( pData->isEOL )
            stl.EOL(pData->bEOL);

        if( pData->isSize )
            stl.Size(pData->iSize);
        if( pData->isFont )
            stl.Font(pData->sFont);
        if( pData->isFore )
            stl.Fore(pData->clrFore);
        if( pData->isBack )
            stl.Back(pData->clrBack);
    }
}

void CColorsDlg::UpdateControls() {
    edSize.Enable(chkSize.IsChecked());
    cmbFont.Enable(chkFont.IsChecked());
    btnFore.Enable(chkFore.IsChecked());
    btnBack.Enable(chkBack.IsChecked());
}

void CColorsDlg::getData(HTREEITEM hItem) {
    if( !hItem ) return;
    auto* pData = (CItemData*) tree.GetItemData(hItem);
    if( !pData ) return;

    int is, b;
    chkBold.GetCheck(is, b);
    pData->isBold = is;
    pData->bBold = b;
    chkItalic.GetCheck(is, b);
    pData->isItalic = is;
    pData->bItalic = b;
    chkUnder.GetCheck(is, b);
    pData->isUnderline = is;
    pData->bUnderline = b;
    chkEOL.GetCheck(is, b);
    pData->isEOL = is;
    pData->bEOL = b;

    pData->isFont = chkFont.IsChecked();
    pData->isSize = chkSize.IsChecked();
    pData->isFore = chkFore.IsChecked();
    pData->isBack = chkBack.IsChecked();

    pData->sFont = cmbFont.GetText();
    pData->iSize = stoi(edSize.GetText());
    pData->clrFore = str2clr(btnFore.GetText());
    pData->clrBack = str2clr(btnBack.GetText());
}

void CColorsDlg::setData(HTREEITEM hItem) {
    if( !hItem ) return;
    auto* pData = (CItemData*) tree.GetItemData(hItem);
    if( !pData ) return;

    m_in = true;
    chkBold.SetCheck(pData->isBold, pData->bBold);
    chkItalic.SetCheck(pData->isItalic, pData->bItalic);
    chkUnder.SetCheck(pData->isUnderline, pData->bUnderline);
    chkEOL.SetCheck(pData->isEOL, pData->bEOL);

    chkSize.SetCheck(pData->isSize);
    chkFont.SetCheck(pData->isFont);
    chkFore.SetCheck(pData->isFore);
    chkBack.SetCheck(pData->isBack);

    edSize.SetText(itoa(pData->iSize));
    cmbFont.SetupCombo(pData->sFont);
    btnFore.SetText(clr2str(pData->clrFore));
    btnBack.SetText(clr2str(pData->clrBack));
    m_in = false;

    UpdateControls();
    apply2Sample(hItem);
}

void CColorsDlg::onNotify(LPARAM lParam) {
    auto* pNMHdr = (NMHDR*) lParam;
    if( pNMHdr->hwndFrom == tree.hWnd() ) {
        NMTREEVIEW* pNM = (NMTREEVIEW*) lParam;
        if( pNMHdr->code == TVN_DELETEITEM )
            onDeleteItem(pNM->itemOld.hItem);
        else if( pNMHdr->code == TVN_SELCHANGED )
            setData(pNM->itemNew.hItem);
    }
}

BOOL Paint_OwnerDrawButton(DRAWITEMSTRUCT* pDIS); // dlgIndi.cpp

BOOL CColorsDlg::onDrawClrBtn(DRAWITEMSTRUCT* pDIS) {
    return Paint_OwnerDrawButton(pDIS);
}
