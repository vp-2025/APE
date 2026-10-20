#include "stdafx.h"
#include "tabs.h"
#include "resource.h"
#include "options.h"
#include "theme.h"

// res\ilCloseBtn.bmp is a strip of 4 images (normal/selected, hover, pressed),
// authored at CLOSE_BTN_BASE. They are stretched to CLOSE_BTN_SIZE, so the X is
// drawn bigger (and it follows the dpi; the layout reads the real image-list
// size back, so drawing and the hit test stay in sync).
const int CLOSE_BTN_BASE = 14;
const int CLOSE_BTN_SIZE = 18;

int closeBtnSize() { return g_dpi.scale(CLOSE_BTN_SIZE); }

HIMAGELIST createCloseIL(int nImages) {
    const int cx = closeBtnSize(), cy = cx;

    HBITMAP hSrc = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_CLOSEBTN));
    if( !hSrc )
        return ImageList_Create(cx, cy, ILC_COLOR24 | ILC_MASK, nImages, 0);

    BITMAP bm{};
    GetObject(hSrc, sizeof(bm), &bm);
    const int srcW = bm.bmWidth / nImages, srcH = bm.bmHeight;

    HDC hScreen = GetDC(nullptr);
    HDC hSrcDC = CreateCompatibleDC(hScreen);
    HDC hDstDC = CreateCompatibleDC(hScreen);
    HBITMAP hDst = CreateCompatibleBitmap(hScreen, cx * nImages, cy);
    HGDIOBJ hOldSrc = SelectObject(hSrcDC, hSrc);
    HGDIOBJ hOldDst = SelectObject(hDstDC, hDst);
    SetStretchBltMode(hDstDC, COLORONCOLOR); // keep the pixels (and the mask) exact, no halo
    for( int i = 0; i < nImages; i++ )
        StretchBlt(hDstDC, i * cx, 0, cx, cy, hSrcDC, i * srcW, 0, srcW, srcH, SRCCOPY);
    SelectObject(hDstDC, hOldDst);
    SelectObject(hSrcDC, hOldSrc);
    DeleteDC(hDstDC);
    DeleteDC(hSrcDC);
    ReleaseDC(nullptr, hScreen);
    DeleteObject(hSrc);

    HIMAGELIST hIL = ImageList_Create(cx, cy, ILC_COLOR24 | ILC_MASK, nImages, 0);
    ImageList_AddMasked(hIL, hDst, 0xFFFFFF);
    DeleteObject(hDst);
    return hIL;
}

void CTabCtrlAdv::applyOptions(bool bSelf) {
    if( g_options.bTabsCloseBtn )
        // the X sits in the right padding: keep room for its (grown) size
        SetPadding(g_dpi.scale(9) + (closeBtnSize() - g_dpi.scale(CLOSE_BTN_BASE)), g_dpi.scale(4));
    else
        SetPadding(g_dpi.scale(4), g_dpi.scale(4));
    if( !bSelf )
        TabCtrl_SetMinTabWidth(m_hWnd, -1);
}

void CTabCtrlAdv::Create(HWND hParent, int x, int y, int w, int h, DWORD id) {
    m_id = id;
    m_hWnd = CreateWindowEx(0, WC_TABCONTROL, "",
                            WS_CHILD | TCS_FOCUSNEVER | TCS_OWNERDRAWFIXED | TCS_TABS | TCS_TOOLTIPS,
                            x, y, w, h, hParent, (HMENU) (UINT_PTR) id, g_hInst, 0);
    auto hFont = (HFONT) ::GetStockObject(DEFAULT_GUI_FONT);
    SetFont(hFont);

    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR) this);
    m_defWndProc = (WNDPROC) SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR) WndProcStatic);

    m_ilCloseBtn = createCloseIL(4);

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
            else if( g_options.bTabsCloseBtn ) {
                if( !m_bTrackLeave ) { // ask to be told when the mouse leaves the control
                    TRACKMOUSEEVENT tme{ sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
                    TrackMouseEvent(&tme);
                    m_bTrackLeave = true;
                }
                CloseBtnHover(iTab, pt);
            }
        }
            break;
        case WM_MOUSELEAVE:
            m_bTrackLeave = false;
            if( !m_isDragging && m_iPressedTab == -1 )
                CloseBtnHover(-1, CPoint(-1, -1)); // hide the X button
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

        case WM_ERASEBKGND:
            if( g_theme.isDark() )
                return TRUE;
            break;
        case WM_PAINT:
            if( g_theme.isDark() ) {
                PaintDark();
                return 0;
            }
            break;

        case WM_TIMER: // vp: sometimes when mouse out closeBtn it still is hovered; this is timer-fix
            if( !m_isDragging && m_iHoverTab != -1 ) {
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

// Geometry of one owner-drawn tab item, relative to the item rect.
// Kept in one place, so the painting and the close-button hit test cannot drift
// apart, and everything scales with the dpi (icons are as big as the image list
// says, the gaps around them are scaled).
struct CTabItemLayout {
    int iconX{}, iconY{};                   // file type icon
    int textL{}, textT{}, textR{}, textB{}; // file name
    int closeX{}, closeY{}, closeW{}, closeH{};
};

CTabItemLayout calcTabItemLayout( const CRect& rc, bool bIcon, const CSize& szIcon, bool bClose, const CSize& szClose ) {
    CTabItemLayout l;

    int xTextL = rc.left;
    if( bIcon ) {
        l.iconX = rc.left + g_dpi.scale(2);
        l.iconY = rc.top + g_dpi.scale(4);
        xTextL = l.iconX + szIcon.cx + g_dpi.scale(4);   // icon + space before the file name
    }

    int xTextR = rc.right;
    if( bClose ) {
        l.closeX = rc.right - g_dpi.scale(1) - szClose.cx;
        l.closeY = rc.top + (rc.height() - szClose.cy) / 2;   // centered, whatever the size is
        l.closeW = szClose.cx;
        l.closeH = szClose.cy;
        xTextR = l.closeX - g_dpi.scale(2);
    }

    l.textL = xTextL;
    l.textT = rc.top + g_dpi.scale(4);
    l.textR = xTextR;
    l.textB = rc.bottom;
    return l;
}

void CTabCtrlAdv::DrawItem(DRAWITEMSTRUCT* pDIS, bool bRO, bool bNotExist) {
    HDC hDC = pDIS->hDC;
    CRect r(pDIS->rcItem);
    bool bSelected = pDIS->itemState & ODS_SELECTED;
    int iTab = (int)pDIS->itemID;

	wstrbuf sBuf(MAX_PATH);
    TCITEMW tci{
        .mask = TCIF_TEXT | TCIF_IMAGE,
        .pszText = sBuf.buf(),
        .cchTextMax = sBuf.size() };
    GetItem(iTab, &tci);

    /**int bk =*/ SetBkMode(hDC, TRANSPARENT);
    bool bDark = g_theme.isDark();
    FillSolidRect(hDC, &r, bDark ? (bSelected ? CTheme::clrCtrl : CTheme::clrBack) : GetSysColor(COLOR_BTNFACE));

    if( bSelected ) {
        CRect r2(r);
        r2.top += g_dpi.scale(2);
        r2.right -= g_dpi.scale(1);
        r2.setHeight(g_dpi.scale(4));
        FillSolidRect(hDC, &r2, RGB(128, 212, 178));
    }

    if( m_isDragging && m_iHoverTab == iTab ) {
        CRect r2(r);
        r2.top += g_dpi.scale(2);
        r2.setHeight(g_dpi.scale(4));
        FillSolidRect(hDC, &r2, RGB(0, 0xA0, 0));
    }

    HIMAGELIST hIL = GetImageList();
    bool bIcon = hIL && tci.iImage >= 0;
    CRect rcItem = GetItemRect(iTab);
    CTabItemLayout l = calcTabItemLayout( rcItem, bIcon, iconSize(hIL), g_options.bTabsCloseBtn, iconSize(m_ilCloseBtn) );

    if( bIcon )
        ImageList_Draw(hIL, tci.iImage, hDC, l.iconX, l.iconY, ILD_TRANSPARENT);

    // the X button is only shown for the tab the mouse is over (the space is still
    // reserved in the layout, so the icon / text never move)
    if( g_options.bTabsCloseBtn && !m_isDragging && m_iHoverTab == iTab ) {
        int idCloseImg;
        if( m_iPressedTab == iTab && m_iHoverBtn == iTab )
            idCloseImg = 3;
        else if( m_iPressedTab == -1 && m_iHoverBtn == iTab )
            idCloseImg = 2;
        else
            idCloseImg = bSelected ? 0 : 1;

        ImageList_Draw(m_ilCloseBtn, idCloseImg, hDC, l.closeX, l.closeY, ILD_TRANSPARENT);
    }

    COLORREF clr;
    if( bNotExist ) {
        if( bDark )
            clr = bSelected ? RGB(0xE0, 0xE0, 0xE0) : RGB(0xA0, 0xA0, 0xA0);
        else
            clr = bSelected ? RGB(0, 0, 0) : RGB(0x40, 0x40, 0x40);
    } else if( bRO ) {
        if( bDark )
            clr = bSelected ? RGB(0xFF, 0x70, 0x70) : RGB(0xC0, 0x50, 0x50);
        else
            clr = bSelected ? RGB(0xFF, 0, 0) : RGB(0x80, 0, 0);
    } else {
        if( bDark )
            clr = bSelected ? RGB(0x70, 0xB0, 0xFF) : RGB(0x50, 0x80, 0xC0);
        else
            clr = bSelected ? RGB(0, 0, 0xFF) : RGB(0, 0, 0x80);
    }
    SetTextColor(hDC, clr);

    CRect rText(l.textL, l.textT, l.textR, l.textB);
    HFONT hFontOld;
    if( bNotExist )
        hFontOld = (HFONT) SelectObject(hDC, m_hFontStrikeOut);
    DrawTextW(hDC, tci.pszText, -1, &rText, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    if( bNotExist )
        SelectObject(hDC, hFontOld);
}

void CTabCtrlAdv::PaintDark() {
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(m_hWnd, &ps);
    CRect rc = GetClientRect();

    // double buffer: the close button hover repaints often
    HDC hMem = CreateCompatibleDC(hDC);
    HBITMAP hBmp = CreateCompatibleBitmap(hDC, rc.width(), rc.height());
    HGDIOBJ hOldBmp = SelectObject(hMem, hBmp);
    HGDIOBJ hOldFont = SelectObject(hMem, GetFont());
    FillSolidRect(hMem, &rc, CTheme::clrBack);

    int iSel = GetCurSel();
    for( int i = 0, cnt = GetItemCount(); i < cnt; i++ ) {
        CRect r = GetItemRect(i);
        if( i == iSel ) { // as the tab control does: the selected tab is bigger
            r.left -= g_dpi.scale(2);
            r.right += g_dpi.scale(2);
            r.top -= g_dpi.scale(2);
        }
        DRAWITEMSTRUCT dis{};
        dis.CtlType = ODT_TAB;
        dis.CtlID = m_id;
        dis.itemID = i;
        dis.itemAction = ODA_DRAWENTIRE;
        dis.itemState = i == iSel ? ODS_SELECTED : 0;
        dis.hwndItem = m_hWnd;
        dis.hDC = hMem;
        dis.rcItem = r;
        ::SendMessage(GetParent(), WM_DRAWITEM, m_id, (LPARAM) &dis); // editor knows read-only & deleted files
    }

    CRect rLine(rc.left, rc.bottom - 1, rc.right, rc.bottom);
    FillSolidRect(hMem, &rLine, CTheme::clrBorder);

    BitBlt(hDC, 0, 0, rc.width(), rc.height(), hMem, 0, 0, SRCCOPY);
    SelectObject(hMem, hOldFont);
    SelectObject(hMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hMem);
    EndPaint(m_hWnd, &ps);
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
        InvalidateTabRect(m_iHoverTab);   // drop the hover look / X button
        m_iHoverTab = m_iHoverTabPrev = m_iHoverBtn = -1;
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
    // same geometry as in DrawItem, so the hit test matches the painted X button
    CTabItemLayout l = calcTabItemLayout( r, false, iconSize(nullptr), true, iconSize(m_ilCloseBtn) );
    return CRect( l.closeX, l.closeY, l.closeX + l.closeW, l.closeY + l.closeH );
}

void CTabCtrlAdv::InvalidateCloseBtnRect(int tab) {
    if( tab == -1 ) return;
    CRect r = GetCloseBtnRect(tab);
    InvalidateRect(m_hWnd, &r, FALSE);
}

void CTabCtrlAdv::InvalidateTabRect(int tab) {
    if( tab == -1 ) return;
    CRect r = GetItemRect(tab);
    r.left -= g_dpi.scale(2);   // the selected tab is painted a bit bigger
    r.right += g_dpi.scale(2);
    r.top -= g_dpi.scale(2);
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
    // the mouse is over `tab` (anywhere on it) -> its X button is shown; the X
    // itself is only under the cursor when the point is inside its rect
    int iHoverBtn = (tab != -1 && GetCloseBtnRect(tab).in(pt)) ? tab : -1;
    if( m_iHoverTab != tab ) {
        m_iHoverTabPrev = m_iHoverTab;
        m_iHoverTab = tab;
        m_iHoverBtn = iHoverBtn;
        InvalidateTabRect(m_iHoverTabPrev);  // the X button is shown / hidden
        InvalidateTabRect(m_iHoverTab);
    } else if( m_iHoverBtn != iHoverBtn ) {
        m_iHoverBtn = iHoverBtn;
        InvalidateCloseBtnRect(tab);         // only the X changes its look
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