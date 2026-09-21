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

class CTabCtrlAdv : public CTabCtrl {
    bool m_isDragging;
    WNDPROC m_defWndProc;
    int m_iDragTab, m_iHoverTab, m_iHoverTabPrev, m_iPressedTab;
    HIMAGELIST m_ilCloseBtn{};
    CPoint m_ptDrag;
    bool m_bStartDrag{};
    HFONT m_hFontStrikeOut{}; // todo: destroy font
public:
    int m_id{};
    int iPrevSel;

    CTabCtrlAdv() {
        iPrevSel = m_iDragTab = m_iHoverTab = m_iHoverTabPrev = m_iPressedTab = -1;
        m_isDragging = false;
        m_defWndProc = nullptr;
    };
    void Create(HWND hParent, int x = 0, int y = 0, int w = 0, int h = 0, DWORD id = 0);

    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void DrawItem(DRAWITEMSTRUCT* dis, bool bRO, bool bNotExist);

    void InvalidateOverRect(int tab);

    void DragStart(int iTab);
    void DragOver(int iTab);
    void DragDrop(int iTab);

    CRect GetCloseBtnRect(int tab);
    void InvalidateCloseBtnRect(int tab);

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