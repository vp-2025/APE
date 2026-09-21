#include "stdafx.h"
#include "resource.h"
#include "lang.h"
#include "dlgIndicators.h"
#include "lexerStyles.h"
#include "shared/str.h"

void fillAlpha(CComboBox& cmb) {
    for( int i = 0; i <= 100; i += 10 )
        cmb.AddString(itoa(i) + "%", i);
}

void fillStyles(CComboBox& cmb) {
    cmb.AddString("Plain", INDIC_PLAIN);
    cmb.AddString("Dash", INDIC_DASH);
    cmb.AddString("Dots", INDIC_DOTS);
    cmb.AddString("Diagonal", INDIC_DIAGONAL);
    cmb.AddString("TT", INDIC_TT);
    cmb.AddString("Squiggle", INDIC_SQUIGGLE);
    cmb.AddString("SquiggleLow", INDIC_SQUIGGLELOW);
    cmb.AddString("SquigglePixmap", INDIC_SQUIGGLEPIXMAP);
//	cmb.AddString( "Strike", INDIC_STRIKE );
    cmb.AddString("Box", INDIC_BOX);
    cmb.AddString("DotBox", INDIC_DOTBOX);
    cmb.AddString("StraightBox", INDIC_STRAIGHTBOX);
    cmb.AddString("RoundBox", INDIC_ROUNDBOX);
    cmb.AddString("FullBox", INDIC_FULLBOX);
    cmb.AddString("CompositionThin", INDIC_COMPOSITIONTHIN);
    cmb.AddString("CompositionThick", INDIC_COMPOSITIONTHICK);
    cmb.AddString("TextFore", INDIC_TEXTFORE);
}

#define COUNT 20
#define ID_STYLE 100
#define ID_COLOR 200
#define ID_ALPHA 300
#define ID_OUTLINE_ALPHA 400

void setParentFont2Children(HWND hDlg) {
    WPARAM font = SendMessage(hDlg, WM_GETFONT, 0, 0);
    for( HWND hCtrl = hDlg; hCtrl != nullptr; hCtrl = GetWindow(hCtrl, hDlg == hCtrl ? GW_CHILD : GW_HWNDNEXT) ) {
        SendMessage(hCtrl, WM_SETFONT, font, 0);
    }
}

/////////////////////////////////////////////////////////////////////

void CIndicatorsDlg::onInit() {
    float dpi = GetScaleDPI();
    int top = 10*dpi, h = 20*dpi;

    CStatic lbl;
    lbl.Create(m_hWnd, 90*dpi, top, 120*dpi, h, "Style", 0, WS_VISIBLE | WS_CHILD | SS_CENTER);
    lbl.Create(m_hWnd, 220*dpi, top, 80*dpi, h, "Color", 0, WS_VISIBLE | WS_CHILD | SS_CENTER);
    lbl.Create(m_hWnd, 310*dpi, top, 50*dpi, h, "Alpha", 0, WS_VISIBLE | WS_CHILD | SS_CENTER);
    lbl.Create(m_hWnd, 370*dpi, top - 10*dpi, 50*dpi, h + 20*dpi, "Outline Alpha", 0, WS_VISIBLE | WS_CHILD | SS_CENTER);
    top += 20*dpi;

    string sText;
    for( int i = 0; g_indicators[i].szName; i++, top += 24*dpi ) {
        if( i ) sText << '\n';
        sText << ' ' << g_indicators[i].szName << ' ';

        vIndi.emplace_back(g_indicators[i], &sci, i);
        CIndiData& data = vIndi.back();

        data.stcLabel.Create(m_hWnd, 10*dpi, top + 2, 80*dpi, h - 4, (string(g_indicators[i].szName) + ":").c_str(), 0, WS_VISIBLE | WS_CHILD);
        data.cmbStyle.Create(m_hWnd, 90*dpi, top, 120*dpi, h, ID_STYLE + i, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST);
        data.btnColor.Create(m_hWnd, 220*dpi, top, 80*dpi, h, "", ID_COLOR + i, WS_VISIBLE | WS_CHILD | BS_OWNERDRAW);
        data.cmbAlpha.Create(m_hWnd, 310*dpi, top, 50*dpi, h, ID_ALPHA + i, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST);
        data.cmbOutlineAlpha.Create(m_hWnd, 370*dpi, top, 50*dpi, h, ID_OUTLINE_ALPHA + i, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST);

        fillAlpha(data.cmbAlpha);
        fillAlpha(data.cmbOutlineAlpha);
        fillStyles(data.cmbStyle);
        data.setData();

/*		data.sci.Create( m_hWnd, 430, top-2, 150, h+4 );
		data.sci.ViewNums( false );
		data.sci.ViewFold( false );
		data.sci.SetHScrollBar( false );
		data.sci.SetVScrollbar( false );
		data.sci.SetCarretLineVisible( false );
		data.sci.AddText(g_indicators[i].szName);
		data.sci.SetReadOnly(true);

		SetupLexerStyles( data.sci );
		data.sci.SetIndicatorCurrent( 0 );
		data.sci.IndicatorFillRange( 0, data.sci.GetLength() );
		data.apply2Sample();
		::ShowWindow( data.sci.hWnd(), SW_SHOW );*/

    }

    sci.Create(m_hWnd);
    sci.ViewNums(false);
    sci.ViewFold(false);
    sci.SetHScrollBar(false);
    sci.SetVScrollbar(false);
    sci.SetCarretLineVisible(false);
    sci.Style(0).Font("Tahoma").Size(10);
    sci.Call(SCI_SETEXTRAASCENT, 3);
    sci.Call(SCI_SETEXTRADESCENT, 4);
    sci.AddText(sText);
    sci.SetReadOnly(true);
    for( const CIndiData& indi: vIndi ) {
        int start = sci.GetLineStartPosition(indi.id);
        int end = sci.GetLineEndPosition(indi.id);
        sci.setIndicator(indi.id, start, end - start);
        indi.apply2Sample();
    }
    ::MoveWindow(sci.hWnd(), 430*dpi, 25*dpi, 150*dpi, top - 20*dpi, TRUE);
    ::ShowWindow(sci.hWnd(), SW_SHOW);

    setParentFont2Children(m_hWnd);
    CenterWindow(m_hWnd);
    g_lang.translateDlg(m_hWnd);
}

void CIndicatorsDlg::onCommand(int cmd, int code) {
    if( cmd >= ID_STYLE && cmd <= ID_STYLE + COUNT ) {
        CIndiData& item = vIndi[cmd - ID_STYLE];
        item.getData();
        item.apply2Sample();
    } else if( cmd >= ID_COLOR && cmd < ID_COLOR + COUNT ) {
        CIndiData& item = vIndi[cmd - ID_COLOR];
        if( dlgChooseColor(m_hWnd, item.color) ) {
            item.btnColor.SetText(clr2str(item.color));
            item.getData();
            item.apply2Sample();
        }
    } else if( cmd >= ID_ALPHA && cmd <= ID_ALPHA + COUNT ) {
        CIndiData& item = vIndi[cmd - ID_ALPHA];
        item.getData();
        item.apply2Sample();
    } else if( cmd >= ID_OUTLINE_ALPHA && cmd <= ID_OUTLINE_ALPHA + COUNT ) {
        CIndiData& item = vIndi[cmd - ID_OUTLINE_ALPHA];
        item.getData();
        item.apply2Sample();
    } else
        switch( cmd ) {
            case IDOK:
                for(const auto& indi : vIndi)
                    indi.store();
                WriteLexerStyles();
            case IDCANCEL:
                EndDialog(m_hWnd, cmd);
            default:
                return;
        }
}

BOOL Paint_OwnerDrawButton(DRAWITEMSTRUCT* pDIS) {
    if( pDIS->CtlType != ODT_BUTTON )
        return FALSE;
    HDC hDC = pDIS->hDC;
    HWND hBtn = pDIS->hwndItem;
    bool isDisabled = (pDIS->itemState & ODS_DISABLED) != 0;
    bool isPressed = (pDIS->itemState & ODS_SELECTED) != 0;
    CRect rBtn = pDIS->rcItem;

    string sClr = GetWindowTextStr(hBtn);
    DWORD clr = (isDisabled || sClr.empty()) ? GetSysColor(COLOR_BTNFACE) : str2clr(sClr);

    FrameRect(hDC, &rBtn, CBrushCache::color2brush(isDarkColor(clr) ? clrWhite : clrBlack));
    InflateRect(&rBtn, -1, -1);
    FillRect(hDC, &rBtn, CBrushCache::color2brush(clr));

    if( isPressed )
        OffsetRect(&rBtn, 1, 1);

    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, isDarkColor(clr) ? clrWhite : clrBlack);
    DrawText(hDC, sClr.c_str(), (int)sClr.size(), &rBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    return TRUE;
}

BOOL CIndicatorsDlg::onDrawClrBtn(DRAWITEMSTRUCT* pDIS) {
    return Paint_OwnerDrawButton(pDIS);
}
