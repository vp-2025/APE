#pragma once

#include "shared/win.h"
#include "shared/platform.h"
#include "shared/str.h"
#include <shellapi.h>

#define WM_TAB_CHANGING     (WM_USER+1)
#define WM_TAB_CHANGED      (WM_USER+2)
#define WM_TAB_REORDER      (WM_USER+3)
#define WM_TAB_CLOSE        (WM_USER+4)
#define WM_TAB_MOVE2NEWAPE  (WM_USER+5)

// Geometry of one owner-drawn tab item, relative to the item rect.
// Kept in one place, so the painting and the close-button hit test cannot drift
// apart, and everything scales with the dpi (icons are as big as the image list
// says, the gaps around them are scaled).
struct CTabItemLayout {
    int iconX{}, iconY{};                   // file type icon
    int textL{}, textT{}, textR{}, textB{}; // file name
    int closeX{}, closeY{}, closeW{}, closeH{};
};

inline CTabItemLayout calcTabItemLayout( const CRect& rc,
                                         bool bIcon, const CSize& szIcon,
                                         bool bClose, const CSize& szClose )
{
    CTabItemLayout l;

    int xTextL = rc.left;
    if( bIcon ) {
        l.iconX = rc.left + g_dpi.scale(2);
        l.iconY = rc.top + g_dpi.scale(3);
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
    l.textT = rc.top + g_dpi.scale(8);
    l.textR = xTextR;
    l.textB = rc.bottom;
    return l;
}

class CTabCtrlAdv : public CTabCtrl {
    bool m_isDragging;
    WNDPROC m_defWndProc;
    // m_iHoverTab = tab under the mouse (its X button is shown), m_iHoverBtn = tab
    // whose X button is under the mouse (hover/pressed look), m_iPressedTab = X held
    int m_iDragTab, m_iHoverTab, m_iHoverTabPrev, m_iPressedTab, m_iHoverBtn;
    bool m_bTrackLeave{};
    HIMAGELIST m_ilCloseBtn{};
    CPoint m_ptDrag;
    bool m_bStartDrag{};
    HFONT m_hFontStrikeOut{}; // todo: destroy font

    CSize iconSize(HIMAGELIST hIL) const {  // real image size (16x16 at 100%)
        int cx = g_dpi.scale(16), cy = g_dpi.scale(16);
        if( hIL )
            ImageList_GetIconSize(hIL, &cx, &cy);
        return CSize(cx, cy);
    }
public:
    int m_id{};
    int iPrevSel;

    CTabCtrlAdv() {
        iPrevSel = m_iDragTab = m_iHoverTab = m_iHoverTabPrev = m_iPressedTab = m_iHoverBtn = -1;
        m_isDragging = false;
        m_defWndProc = nullptr;
    };
    void Create(HWND hParent, int x = 0, int y = 0, int w = 0, int h = 0, DWORD id = 0);

    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void DrawItem(DRAWITEMSTRUCT* dis, bool bRO, bool bNotExist);
    void PaintDark(); // whole control, the themed tab control draws light borders

    void InvalidateOverRect(int tab);

    void DragStart(int iTab);
    void DragOver(int iTab);
    void DragDrop(int iTab);

    CRect GetCloseBtnRect(int tab);
    void InvalidateCloseBtnRect(int tab);
    void InvalidateTabRect(int tab);

    bool CloseBtnDown(int tab, const CPoint& pt);
    void CloseBtnHover(int tab, const CPoint& pt);
    void CloseBtnUp(int tab, const CPoint& pt);

    void applyOptions(bool bSelf = false);
};

class CShellImageList {
    HIMAGELIST m_hIL;
    typedef std::map<std::string, int> mtype;
    mtype m;
public:
    CShellImageList() {
        SHFILEINFO sfi = {nullptr};
        m_hIL = (HIMAGELIST) SHGetFileInfo("", 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
    }
    explicit operator HIMAGELIST() const { return m_hIL; }
    int getFileTypeIndex(const std::string& sFile) const {
        std::string sExt = '.' + toLowerVal(ExtractFileExt(sFile));
        auto it = m.find(sExt);
        return it != m.end() ? it->second : -1;
    }
    int getFileTypeIndex(const std::string& sFile) {
        std::string sExt = '.' + toLowerVal(ExtractFileExt(sFile));
        auto it = m.find(sExt);
        if( it != m.end() )
            return it->second;
        SHFILEINFO sfi = {nullptr};
        SHGetFileInfo(sExt.c_str(), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
        m[sExt] = sfi.iIcon;
        return sfi.iIcon;
    }
};