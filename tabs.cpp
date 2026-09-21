#include "stdafx.h"
#include "tabs.h"
#include "resource.h"
#include "options.h"

void CTabCtrlAdv::applyOptions(bool bSelf) {
    if( g_options.bTabsCloseBtn )
        SetPadding(9, 4);
    else
        SetPadding(4, 4);
    if( !bSelf )
        TabCtrl_SetMinTabWidth(m_hWnd, -1);
}

void CTabCtrlAdv::Create(HWND hParent, int x, int y, int w, int h, DWORD id) {
    m_id = id;
    m_hWnd = CreateWindowEx(0, WC_TABCONTROL, "",
                            WS_CHILD | TCS_FOCUSNEVER | TCS_OWNERDRAWFIXED | TCS_TABS | TCS_TOOLTIPS,
                            x, y, w, h, hParent, (HMENU) id, g_hInst, 0);
    auto hFont = (HFONT) ::GetStockObject(DEFAULT_GUI_FONT);
    SetFont(hFont);

    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR) this);
    m_defWndProc = (WNDPROC) SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR) WndProcStatic);

    m_ilCloseBtn = ImageList_Create(14, 14, ILC_COLOR24 | ILC_MASK, 4, 0);
    ImageList_AddMasked(m_ilCloseBtn, LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_CLOSEBTN)), 0xFFFFFF);

    applyOptions(true);

    SetTimer(1, 1000);

    {
        LOGFONT lFont = {0};
        ::GetObject(hFont, sizeof(lFont), &lFont);
        lFont.lfStrikeOut = TRUE;
        m_hFontStrikeOut = CreateFontIndirect(&lFont);
    }
}

LRESULT CALLBACK CTabCtrlAdv::WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* pTab = (CTabCtrlAdv*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if( pTab )
        return pTab->WndProc(hwnd, msg, wParam, lParam);
    else
        return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CTabCtrlAdv::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch( msg ) {
        case WM_LBUTTONDOWN: {
            CPoint pt(LOWORD(lParam), HIWORD(lParam));
            int iTab = getItemIndexAt(pt);
            if( g_options.bTabsCloseBtn && CloseBtnDown(iTab, pt) )
                return TRUE;
            if( g_options.bTabsDragNDrop && m_iPressedTab == -1 && ::DragDetect(m_hWnd, pt) )
                DragStart(iTab);
        }
            break;
        case WM_MOUSEMOVE: {
            CPoint pt(LOWORD(lParam), HIWORD(lParam));
            int iTab = getItemIndexAt(pt);
            if( g_options.bTabsDragNDrop && m_isDragging )
                DragOver(iTab);
            else if( g_options.bTabsCloseBtn )
                CloseBtnHover(iTab, pt);
        }
            break;
        case WM_LBUTTONUP: {
            CPoint pt(LOWORD(lParam), HIWORD(lParam));
            int iTab = getItemIndexAt(pt);
            if( g_options.bTabsDragNDrop && m_isDragging )
                DragDrop(iTab);
            else if( g_options.bTabsCloseBtn )
                CloseBtnUp(iTab, pt);
        }
            break;

        case WM_TIMER: // vp: sometimes when mouse out closeBtn it still is hovered; this is timer-fix
            if( m_iHoverTab != -1 ) {
                CPoint pt = GetCursorPos();
                ScreenToClient(pt);
                int iTab = getItemIndexAt(pt);
                if( m_iHoverTab != iTab )
                    CloseBtnHover(iTab, pt);
            }
            break;
    }
    return ::CallWindowProc(m_defWndProc, hwnd, msg, wParam, lParam);
}

void FillSolidRect(HDC hDC, RECT* pR, DWORD clr) {
    SetBkColor(hDC, clr);
    ExtTextOut(hDC, 0, 0, ETO_OPAQUE, pR, nullptr, 0, nullptr);
}

void CTabCtrlAdv::DrawItem(DRAWITEMSTRUCT* pDIS, bool bRO, bool bNotExist) {
    HDC hDC = pDIS->hDC;
    CRect r(pDIS->rcItem);
    bool bSelected = pDIS->itemState & ODS_SELECTED;
    int iTab = (int)pDIS->itemID;

	wstrbuf sBuf(MAX_PATH);
    TCITEMW tci{};
    tci.mask = TCIF_TEXT | TCIF_IMAGE;
    tci.pszText = sBuf.buf();
    tci.cchTextMax = sBuf.size();
    GetItem(iTab, &tci);

    /**int bk =*/ SetBkMode(hDC, TRANSPARENT);
    FillSolidRect(hDC, &r, GetSysColor(COLOR_BTNFACE));

    if( bSelected ) {
        CRect r2(r);
        r2.top += 2;
        r2.setHeight(4);
        FillSolidRect(hDC, &r2, RGB(0xFA, 0xAA, 0x3C));
    }

    if( m_isDragging && m_iHoverTab == iTab ) {
        CRect r2(r);
        r2.top += 2;
        r2.setHeight(4);
        FillSolidRect(hDC, &r2, RGB(0, 0xA0, 0));
    }

    if( bSelected ) {
        r.left += 4;
        r.right -= 4;
        r.top += 2;
        r.bottom -= 4;
    }

    HIMAGELIST hIL = GetImageList();
    if( hIL && tci.iImage >= 0 ) {
        r.left += 2;
        ImageList_Draw(hIL, tci.iImage, hDC, r.left, r.top + 3, ILD_TRANSPARENT);
        r.left += 16;
        r.left += 2;
    }

    if( g_options.bTabsCloseBtn ) {
        int idCloseImg;
        if( !m_isDragging && m_iPressedTab == iTab && m_iHoverTab == iTab )
            idCloseImg = 3;
        else if( !m_isDragging && m_iPressedTab == -1 && m_iHoverTab == iTab )
            idCloseImg = 2;
        else
            idCloseImg = bSelected ? 0 : 1;

        r.right -= 1;
        r.right -= 14;
        ImageList_Draw(m_ilCloseBtn, idCloseImg, hDC, r.right, r.top + 4, ILD_TRANSPARENT);
        r.right -= 2;
    }

    r.top += 4;
    COLORREF clr;
    if( bNotExist ) {
        clr = bSelected ? RGB(0, 0, 0) : RGB(0x40, 0x40, 0x40);
    } else if( bRO ) {
        clr = bSelected ? RGB(0xFF, 0, 0) : RGB(0x80, 0, 0);
    } else {
        clr = bSelected ? RGB(0, 0, 0xFF) : RGB(0, 0, 0x80);
    }
    SetTextColor(hDC, clr);

    HFONT hFontOld;
    if( bNotExist )
        hFontOld = (HFONT) SelectObject(hDC, m_hFontStrikeOut);
    DrawTextW(hDC, tci.pszText, -1, &r, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    if( bNotExist )
        SelectObject(hDC, hFontOld);
}

void CTabCtrlAdv::InvalidateOverRect(int tab) {
    if( tab == -1 ) return;
    CRect r = GetItemRect(tab);
    r.top += 2;
    r.setHeight(4);
    InvalidateRect(m_hWnd, &r, FALSE);
}

void CTabCtrlAdv::DragStart(int iTab) {
    if( iTab != -1 ) {
        if( iTab != GetCurSel() ) {
            ::SendMessage(GetParent(), WM_TAB_CHANGING, 0, 0); // tell Editor tab is changing
            SetCurSel(iTab);
            ::SendMessage(GetParent(), WM_TAB_CHANGED, 0, 0); // tell Editor tab is changed
        }
        m_iDragTab = iTab;
        m_isDragging = true;
        SetCapture(m_hWnd);
        m_ptDrag = GetCursorPos();
        m_bStartDrag = false;
    }
}

void CTabCtrlAdv::DragOver(int iTab) {
    if( !m_bStartDrag && m_ptDrag == GetCursorPos() )
        return;
    m_bStartDrag = true;
    bool bOver = /*iTab!=-1 &&*/ iTab != m_iDragTab;
    SetCursor(LoadCursor(nullptr, bOver ? IDC_HAND : IDC_NO));
    m_iHoverTabPrev = m_iHoverTab;
    m_iHoverTab = bOver ? iTab : -1;
    if( m_iHoverTabPrev != m_iHoverTab ) {
        InvalidateOverRect(m_iHoverTab);
        InvalidateOverRect(m_iHoverTabPrev);
    }
}

void CTabCtrlAdv::DragDrop(int iTab) {
    m_isDragging = false;
    ReleaseCapture();
    SetCursor(LoadCursor(nullptr, IDC_ARROW));

    if( iTab == -1 ) {
        ::SendMessage(GetParent(), WM_TAB_MOVE2NEWAPE, 0, 0);
    } else if( iTab != m_iDragTab ) {
        // mark tabs before reorder
        for( int i = 0, cnt = GetItemCount(); i < cnt; i++ )
            SetItemParam(i, i);

        TCITEMW itDrag, itShift;
        itDrag.mask = itShift.mask = TCIF_IMAGE | TCIF_TEXT | TCIF_PARAM;

	    wstrbuf sBuf1(MAX_PATH);
        itDrag.pszText = sBuf1.buf();
        itDrag.cchTextMax = sBuf1.size();

	    wstrbuf sBuf2(MAX_PATH);
        itShift.pszText = sBuf2.buf();
        itShift.cchTextMax = sBuf2.size();

        GetItem(m_iDragTab, &itDrag);
        if( m_iDragTab > iTab ) {
            for( int i = m_iDragTab; i > iTab; i-- ) {
                GetItem(i - 1, &itShift);
                SetItem(i, &itShift);
            }
        } else {
            for( int i = m_iDragTab; i < iTab; i++ ) {
                GetItem(i + 1, &itShift);
                SetItem(i, &itShift);
            }
        }
        SetItem(iTab, &itDrag);

        SetCurSel(iTab);

        // change order of tabs
        ::SendMessage(GetParent(), WM_TAB_REORDER, 0, 0);
    }

    m_iDragTab = -1;
}

CRect CTabCtrlAdv::GetCloseBtnRect(int tab) {
    CRect r = GetItemRect(tab);
    r.right -= 2;

    r.left = r.right - 14 - 1;
    r.top += 4;
    r.setWidth(14);
    r.setHeight(14);
    return r;
}

void CTabCtrlAdv::InvalidateCloseBtnRect(int tab) {
    if( tab == -1 ) return;
    CRect r = GetCloseBtnRect(tab);
    InvalidateRect(m_hWnd, &r, FALSE);
}

bool CTabCtrlAdv::CloseBtnDown(int tab, const CPoint& pt) {
    m_iPressedTab = GetCloseBtnRect(tab).in(pt) ? tab : -1;
    if( m_iPressedTab >= 0 ) {
        InvalidateCloseBtnRect(m_iPressedTab);
        SetCapture(m_hWnd);
    }
    return m_iPressedTab >= 0;
}

void CTabCtrlAdv::CloseBtnHover(int tab, const CPoint& pt) {
    m_iHoverTabPrev = m_iHoverTab;
    m_iHoverTab = GetCloseBtnRect(tab).in(pt) ? tab : -1;
    if( m_iHoverTabPrev != m_iHoverTab ) {
        InvalidateCloseBtnRect(m_iHoverTab);
        InvalidateCloseBtnRect(m_iHoverTabPrev);
    }
}

void CTabCtrlAdv::CloseBtnUp(int tab, const CPoint& pt) {
    ReleaseCapture();
    int iTab = m_iPressedTab;
    m_iPressedTab = -1;
    if( iTab == tab && GetCloseBtnRect(tab).in(pt) ) {
        InvalidateCloseBtnRect(iTab);
        ::SendMessage(GetParent(), WM_TAB_CLOSE, 0, iTab); // tell Editor tab is closed
    }
}