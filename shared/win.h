#pragma once

/*
	wrapper classes for WinAPI

c++ code is MBCS (ansi/utf8)
WinApi calls ansi or wide: depends what is needed
 1.base
 2.ctrls
 3.menu
 4.registry
 5.iniFile
 6.acx
*/

#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>

#include <string>
#include <vector>
#include <map>
#include <set>

#include "str.h"

extern HKEY g_hKey;
extern const char *g_szPath;
extern HINSTANCE g_hInst;

void CenterWindow(HWND hWnd);

void CenterWindowOnDesktop(HWND hWnd);

inline std::string GetWindowTextStr(HWND hWnd) {
    int len = GetWindowTextLengthA(hWnd);
    strbuf sBuf(len + 1); // +1 for ending zero
    len = GetWindowTextA(hWnd, sBuf.buf(), sBuf.size());
    return sBuf.str(len); // len without ending zero
}

inline void SetWindowTextStr(HWND hWnd, const std::string &s) {
    SetWindowTextA(hWnd, s.c_str());
}

inline std::wstring GetWindowTextStrW(HWND hWnd) {
    int len = GetWindowTextLengthW(hWnd);
    wstrbuf sBuf(len + 1); // +1 for ending zero
    len = GetWindowTextW(hWnd, sBuf.buf(), sBuf.size());
    return sBuf.str(len); // len without ending zero
}

inline void SetWindowTextStr(HWND hWnd, const std::wstring &ws) {
    SetWindowTextW(hWnd, ws.c_str());
}

inline std::string GetMenuText(HMENU hMenu, int cmd, int flags) {
    int len = GetMenuStringA(hMenu, cmd, nullptr, 0, flags);
    strbuf sBuf(len + 1); // +1 for ending zero
    len = GetMenuStringA(hMenu, cmd, sBuf.buf(), sBuf.size(), flags);
    return sBuf.str(len); // len without ending zero
}

inline std::wstring GetMenuTextW(HMENU hMenu, int cmd, int flags) {
    int len = GetMenuStringW(hMenu, cmd, nullptr, 0, flags);
    wstrbuf sBuf(len + 1); // +1 for ending zero
    len = GetMenuStringW(hMenu, cmd, sBuf.buf(), sBuf.size(), flags);
    return sBuf.str(len); // len without ending zero
}

inline std::wstring getClassNameW(HWND hWnd) {
    wstrbuf sBuf(MAX_PATH);
    size_t len = ::GetClassNameW(hWnd, sBuf.buf(), sBuf.size());
    return sBuf.str(len); // len without ending zero
}

inline float GetScaleDPI() {
    HDC hdc = ::CreateCompatibleDC(nullptr);
    float dpiScale = (float) ::GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
    ::DeleteDC(hdc);
    return dpiScale;
}

inline std::string itoa(int i) {
    return std::to_string(i);
}

/////////////////////////////////////////////////////////////////////
// base classes
/////////////////////////////////////////////////////////////////////

struct CPoint : public POINT {
    CPoint() {
        x = y = 0;
    }

    CPoint(int x1, int y1) {
        x = x1;
        y = y1;
    }

    bool operator==(const CPoint &pt) {
        return x == pt.x && y == pt.y;
    }

    bool operator!=(const CPoint &pt) {
        return !(*this == pt);
    }
};

struct CSize : public SIZE {
    CSize() {
        cx = cy = 0;
    }

    CSize(int cx1, int cy1) {
        cx = cx1;
        cy = cy1;
    }

    bool operator==(const CSize &sz) {
        return cx == sz.cx && cy == sz.cy;
    }

    bool operator!=(const CSize &sz) {
        return !(*this == sz);
    }
};

struct CRect : public RECT {
    CRect() {
        left = right = top = bottom = 0;
    }

    CRect(RECT r) {
        left = r.left;
        right = r.right;
        top = r.top;
        bottom = r.bottom;
    }

    CRect(int x, int y, int x2, int y2) {
        left = x;
        top = y;
        right = x2;
        bottom = y2;
    }

    CRect(int x, int y, int w, int h, bool) {
        left = x;
        top = y;
        setWidth(w);
        setHeight(h);
    }

    void setLeft(int x) {
        int w = width();
        left = x;
        setWidth(w);
    }

    void setTop(int y) {
        int h = height();
        top = y;
        setHeight(h);
    }

    void setWidth(int w) {
        right = left + w;
    }

    void setHeight(int h) {
        bottom = top + h;
    }

    int width() const {
        return right - left;
    }

    int height() const {
        return bottom - top;
    }

    CPoint LeftTop() const {
        return {left, top};
    }

    CSize Size() const {
        return {width(), height()};
    }

    bool in(int x, int y) const {
        return left <= x && x <= right && top <= y && y <= bottom;
    }

    bool in(const CPoint &pt) const {
        return left <= pt.x && pt.x <= right && top <= pt.y && pt.y <= bottom;
    }

    bool operator==(const CRect &r) const {
        return left == r.left && right == r.right && top == r.top && bottom == r.bottom;
    }

    bool operator!=(const CRect &r) const {
        return !(*this == r);
    }
};

struct CWindowPlacement : public WINDOWPLACEMENT {
    CWindowPlacement() {
        memset(this, 0, sizeof(WINDOWPLACEMENT));
        length = sizeof(WINDOWPLACEMENT);
    }

    int left() const {
        return rcNormalPosition.left;
    }

    int top() const {
        return rcNormalPosition.top;
    }

    int width() const {
        return rcNormalPosition.right - rcNormalPosition.left;
    }

    int height() const {
        return rcNormalPosition.bottom - rcNormalPosition.top;
    }

    void setLeft(int x) {
        int w = width();
        rcNormalPosition.left = x;
        setWidth(w);
    }

    void setTop(int y) {
        int h = height();
        rcNormalPosition.top = y;
        setHeight(h);
    }

    void setWidth(int w) {
        rcNormalPosition.right = rcNormalPosition.left + w;
    }

    void setHeight(int h) {
        rcNormalPosition.bottom = rcNormalPosition.top + h;
    }
};

class CWaitCursor {
    static int cnt;
public:
    CWaitCursor() {
        if (!cnt) SetCursor(LoadCursor(nullptr, IDC_WAIT));
        cnt++;
    }

    ~CWaitCursor() {
        cnt--;
        if (!cnt) SetCursor(LoadCursor(nullptr, IDC_ARROW));
    }
};

class CFont {
    HFONT hFont;

    void CreateFont(int weight);

public:
    CFont() {
        hFont = nullptr;
    }

    ~CFont() {
        if (hFont) DeleteObject(hFont);
    }

    explicit operator HFONT() {
        return hFont;
    }

    void CreateNormalFont() {
        CreateFont(350);
    }

    void CreateBoldFont() {
        CreateFont(700);
    }
};

inline CPoint GetCursorPos() {
    CPoint pt;
    ::GetCursorPos(&pt);
    return pt;
}

/////////////////////////////////////////////////////////////////////
// ctrls
/////////////////////////////////////////////////////////////////////

class CWindow {
protected:
    HWND m_hWnd;
public:
    CWindow() {
        m_hWnd = nullptr;
    }

    CWindow &Attach(HWND hWnd) {
        m_hWnd = hWnd;
        return *this;
    }

    CWindow &Attach(HWND hDlg, int iCtrl) {
        m_hWnd = ::GetDlgItem(hDlg, iCtrl);
        return *this;
    }

    void Detach() {
        m_hWnd = nullptr;
    }

    void Destroy() {
        if (m_hWnd) ::DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }

    HWND hWnd() const {
        return m_hWnd;
    }

    HWND GetParent() const {
        return ::GetParent(m_hWnd);
    }

    LRESULT SendMessage(UINT msg, WPARAM wPrm = 0, LPARAM lPrm = 0) {
        return ::SendMessageA(m_hWnd, msg, wPrm, lPrm);
    }

    LRESULT SendMessageW(UINT msg, WPARAM wPrm = 0, LPARAM lPrm = 0) {
        return ::SendMessageW(m_hWnd, msg, wPrm, lPrm);
    }

    BOOL PostMessage(UINT msg, WPARAM wPrm = 0, LPARAM lPrm = 0) {
        return ::PostMessageA(m_hWnd, msg, wPrm, lPrm);
    }

    BOOL PostMessageW(UINT msg, WPARAM wPrm = 0, LPARAM lPrm = 0) {
        return ::PostMessageW(m_hWnd, msg, wPrm, lPrm);
    }

    int MessageBox(const std::string &text, const std::string &title = std::string(), UINT uType = MB_OK) {
        return ::MessageBoxA(m_hWnd, text.c_str(), title.c_str(), uType);
    }

    DWORD GetStyle() {
        return (DWORD) ::GetWindowLong(m_hWnd, GWL_STYLE);
    }

    DWORD GetStyleEx() {
        return (DWORD) ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
    }

private:
    void ModifyStyle(int idx, DWORD dwRemove, DWORD dwAdd);

public:
    void ModifyStyle(DWORD dwRemove, DWORD dwAdd) {
        ModifyStyle(GWL_STYLE, dwRemove, dwAdd);
    }

    void ModifyStyleEx(DWORD dwRemove, DWORD dwAdd) {
        ModifyStyle(GWL_EXSTYLE, dwRemove, dwAdd);
    }

    void Enable(bool b) {
        ::EnableWindow(m_hWnd, b);
    }

    bool IsEnabled() {
        return ::IsWindowEnabled(m_hWnd) != 0;
    }

    void SetVisible(bool b) {
        ::ShowWindow(m_hWnd, b ? SW_SHOW : SW_HIDE);
    }

    bool IsVisible() {
        return ::IsWindowVisible(m_hWnd) != 0;
    }

    void SetText(const std::string &str) {
        SetWindowTextStr(m_hWnd, str);
    }

    void SetText(const std::wstring &wstr) {
        SetWindowTextStr(m_hWnd, wstr);
    }

    std::string GetText() {
        return ::GetWindowTextStr(m_hWnd);
    }

    std::wstring GetTextW() {
        return ::GetWindowTextStrW(m_hWnd);
    }

    std::string GetClassName() {
        strbuf sBuf(MAX_PATH);
        size_t len = ::GetClassNameA(m_hWnd, sBuf.buf(), sBuf.size());
        return sBuf.str(len); // len without ending zero
    }

    HFONT GetFont() {
        return (HFONT) SendMessage(WM_GETFONT);
    }

    void SetFont(HFONT hFont) {
        SendMessage(WM_SETFONT, (WPARAM) hFont);
    }

    void SetIconBig(HICON hIco) {
        SendMessage(WM_SETICON, ICON_BIG, (LPARAM) hIco);
    }

    void SetIconSmall(HICON hIco) {
        SendMessage(WM_SETICON, ICON_SMALL, (LPARAM) hIco);
    }

    void Invalidate() {
        ::InvalidateRect(m_hWnd, nullptr, TRUE);
    }

    CRect GetRect() {
        CRect r;
        ::GetWindowRect(m_hWnd, &r);
        return r;
    }

    CRect GetClientRect() {
        CRect r;
        ::GetClientRect(m_hWnd, &r);
        return r;
    }

    int getWidth() {
        return GetRect().width();
    }

    int getHeight() {
        return GetRect().height();
    }

    void ClientToScreen(CPoint &pt) {
        ::ClientToScreen(m_hWnd, &pt);
    }

    void ScreenToClient(CPoint &pt) {
        ::ScreenToClient(m_hWnd, &pt);
    }

    void ShowWindow(int cmd) {
        ::ShowWindow(m_hWnd, cmd);
    }

    void BringWindowToTop() {
        ::BringWindowToTop(m_hWnd);
    }

    void Move(int x, int y, int w, int h) {
        ::MoveWindow(m_hWnd, x, y, w, h, TRUE);
    }

    void Move(const CRect &r) {
        ::MoveWindow(m_hWnd, r.left, r.top, r.width(), r.height(), TRUE);
    }

    void SetFocus() {
        ::SetFocus(m_hWnd);
    }

    void SetTimer(UINT id, UINT elapse) {
        ::SetTimer(m_hWnd, id, elapse, nullptr);
    }

    void KillTimer(UINT id) {
        ::KillTimer(m_hWnd, id);
    }
};

class CStatic : public CWindow {
public:
    bool Create(HWND hParent, int x = 0, int y = 0, int w = 0, int h = 0, const char *szText = nullptr, WORD id = 0,
                DWORD dwStyle = WS_CHILD, DWORD dwStyleEx = 0);
};

class CEdit : public CWindow {
public:
    bool Create(HWND hParent, int x = 0, int y = 0, int w = 0, int h = 0, const char *szText = nullptr, WORD id = 0,
                DWORD dwStyle = WS_CHILD, DWORD dwStyleEx = 0);
};

class CComboBox : public CWindow {
public:
    bool Create(HWND hParent, int x = 0, int y = 0, int w = 0, int h = 0, WORD id = 0, DWORD dwStyle = WS_CHILD,
                DWORD dwStyleEx = 0);

    int GetCount() {
        return (int) SendMessage(CB_GETCOUNT);
    }

    void DeleteString(int idx) {
        SendMessage(CB_DELETESTRING, idx);
    }

    void SetCurSel(int idx) {
        SendMessage(CB_SETCURSEL, idx);
    }

    int GetCurSel() {
        return (int) SendMessage(CB_GETCURSEL);
    }

    void SetItemData(int idx, LPARAM id) {
        SendMessage(CB_SETITEMDATA, idx, id);
    }

    LPARAM GetItemData(int idx) {
        return SendMessage(CB_GETITEMDATA, idx);
    }

    void Clear() {
        while (GetCount()) DeleteString(0);
    }

    int AddString(const std::string &s) {
        return (int) SendMessage(CB_ADDSTRING, 0, (LPARAM) s.c_str());
    }

    int AddString(const std::string &s, int id) {
        int k = AddString(s);
        SetItemData(k, id);
        return k;
    }

    int InsertString(int idx, const std::string &s) {
        return (int) SendMessage(CB_INSERTSTRING, idx, (LPARAM) s.c_str());
    }

    int InsertString(int idx, const std::string &s, int id) {
        int k = InsertString(idx, s);
        SetItemData(k, id);
        return k;
    }

    int FindString(const std::string &s) {
        return SendMessageA(CB_FINDSTRINGEXACT, -1, (LPARAM) s.c_str());
    }

    std::string GetString(int idx);

    int AddString(const std::wstring &s) {
        return (int) SendMessageW(CB_ADDSTRING, 0, (LPARAM) s.c_str());
    }

    int AddString(const std::wstring &s, int id) {
        int k = AddString(s);
        SetItemData(k, id);
        return k;
    }

    int InsertString(int idx, const std::wstring &s) {
        return (int) SendMessageW(CB_INSERTSTRING, idx, (LPARAM) s.c_str());
    }

    int InsertString(int idx, const std::wstring &s, LPARAM id) {
        int k = InsertString(idx, s);
        SetItemData(k, id);
        return k;
    }

    int FindString(const std::wstring &s) {
        return (int) SendMessageW(CB_FINDSTRINGEXACT, -1, (LPARAM) s.c_str());
    }

    std::wstring GetStringW(int idx);

    void SetupCombo(const std::string &s) {
        SetCurSel(FindString(s));
    }

    void SetupCombo(const std::wstring &s) {
        SetCurSel(FindString(s));
    }

    std::string GetCombo() {
        int k = GetCurSel();
        return k != -1 ? GetString(k) : "";
    }

    std::wstring GetComboW() {
        int k = GetCurSel();
        return k != -1 ? GetStringW(k) : L"";
    }

    void SetupComboInt(int idx) {
        SetCurSel(idx);
    }

    int GetComboInt() {
        return GetCurSel();
    }

    void SetupComboEx(LPARAM id);

    LPARAM GetComboEx() {
        int k = GetCurSel();
        return k != -1 ? GetItemData(k) : (LPARAM) -1;
    }

    void UpdateMRU(int cnt = 10); // most recent usage: store last cnt(10) strings
    void LoadCombo(const std::string &sSection);

    void SaveCombo(const std::string &sSection);
};

class CCheckBox : public CWindow {
public:
    bool IsChecked() {
        return SendMessage(BM_GETCHECK) == BST_CHECKED;
    }

    void SetCheck(bool b) {
        SendMessage(BM_SETCHECK, b ? BST_CHECKED : BST_UNCHECKED);
    }

    void GetCheck(int &is, int &b);

    void SetCheck(int is, int b);
};

class CButton : public CWindow {
public:
    bool Create(HWND hParent, int x = 0, int y = 0, int w = 0, int h = 0, const char *szText = nullptr, WORD id = 0,
                DWORD dwStyle = WS_CHILD);
};

class CStatusbar : public CWindow {
    std::vector<int> vWidth;
public:
    bool Create(HWND hParent, WORD id = 0, DWORD dwStyle = WS_CHILD | WS_VISIBLE);

    void SetParts(int cnt, int widths[]) {
        for (int i = 0; i < cnt; i++) {
            if (widths[i] == -1) break;
            if (i) widths[i] += widths[i - 1];
            vWidth.push_back(widths[i]);
        }
        SendMessage(SB_SETPARTS, cnt, (LPARAM) widths);
    }

    int x2part(int x) {
        int left = 0;
        for (size_t i = 0; i < vWidth.size(); left = vWidth[i], i++)
            if (left <= x && x < vWidth[i])
                return (int) i;
        return -1;
    }

    int part2x(int p) {
        return p > 0 ? vWidth[p - 1] : 0;
    }

    void SetText(int idx, const char *sz) {
        SendMessage(SB_SETTEXT, idx, (LPARAM) sz);
    }

    void SetText(int idx, LPCWSTR wsz) {
        SendMessage(SB_SETTEXTW, idx, (LPARAM) wsz);
    }

    void SetText(int idx, const std::string &s) {
        SetText(idx, s.c_str());
    }

    void SetText(int idx, const std::wstring &ws) {
        SetText(idx, ws.c_str());
    }
};

class CToolbar : public CWindow {
public:
    bool Create(HWND hParent, int resID, int cnt, TBBUTTON buttons[], WORD id = 0,
                DWORD dwStyle = WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NODIVIDER);

    void Enable(int cmd, bool b) {
        SendMessage(TB_ENABLEBUTTON, cmd, (LPARAM) MAKELONG(b, 0));
    }

    void Check(int cmd, bool b) {
        SendMessage(TB_CHECKBUTTON, cmd, (LPARAM) MAKELONG(b, 0));
    }
};

class CReBar : public CWindow {
public :
    void Create(HWND hParent);

    void addBand(CWindow &wnd);
};

class CImageList {
    HIMAGELIST m_hIL;
public:
    void Create(int w, int h) {
        m_hIL = ImageList_Create(w, h, ILC_COLOR32 | ILC_MASK, 4, 8);
    }

    void AddIcon(HICON hIco) {
        ImageList_AddIcon(m_hIL, hIco);
    }

    void AddIcon(int iIcoID) {
        HICON hIco = ::LoadIcon(g_hInst, MAKEINTRESOURCE(iIcoID));
        AddIcon(hIco);
        DestroyIcon(hIco);
    }

    HIMAGELIST hIL() const {
        return m_hIL;
    }

    int GetCount() const {
        return ImageList_GetImageCount(m_hIL);
    }
};

class CTreeCtrl : public CWindow {
public:
    bool Create(HWND hParent, DWORD dwStyleAdd = 0);

    HTREEITEM InsertItem(HTREEITEM hParent, const char *sz, void *data = nullptr);

    bool DeleteItem(HTREEITEM hItem) {
        return TreeView_DeleteItem(m_hWnd, hItem) != 0;
    }

    void *GetItemData(HTREEITEM hItem);

    void DeleteAllItems() {
        TreeView_DeleteAllItems(m_hWnd);
    }

    void Expand(HTREEITEM hItem) {
        TreeView_Expand(m_hWnd, hItem, TVE_EXPAND);
    }

    void Collapse(HTREEITEM hItem) {
        TreeView_Expand(m_hWnd, hItem, TVE_COLLAPSE);
    }

    void Toggle(HTREEITEM hItem) {
        TreeView_Expand(m_hWnd, hItem, TVE_TOGGLE);
    }

    HTREEITEM GetRoot() {
        return TreeView_GetRoot(m_hWnd);
    }

    int GetCount() {
        return TreeView_GetCount(m_hWnd);
    }

    HTREEITEM GetParent(HTREEITEM hItem) {
        return TreeView_GetParent(m_hWnd, hItem);
    }

    HTREEITEM GetSelection() {
        return TreeView_GetSelection(m_hWnd);
    }

    void SelectItem(HTREEITEM hItem) {
        TreeView_SelectItem(m_hWnd, hItem);
    }

    HTREEITEM GetChild(HTREEITEM hItem) {
        return TreeView_GetChild(m_hWnd, hItem);
    }

    HTREEITEM GetNext(HTREEITEM hItem) {
        return TreeView_GetNextSibling(m_hWnd, hItem);
    }

    HTREEITEM GetPrev(HTREEITEM hItem) {
        return TreeView_GetPrevSibling(m_hWnd, hItem);
    }
};

#define SELECTED_FOCUSED    (LVIS_SELECTED|LVIS_FOCUSED)

class CListCtrl : public CWindow {
public:
    bool Create(HWND hParent, DWORD dwStyleAdd = 0);

    int InsertItem(int idx, const char *sz = "", void *data = nullptr, int img = -1);

    int InsertItem(int idx, LPCWSTR sz, void *data = nullptr, int img = -1);

    int InsertItem(int idx, const std::string &s, void *data = nullptr, int img = -1) {
        return InsertItem(idx, s.c_str(), data, img);
    }

    int InsertItem(int idx, const std::wstring &s, void *data = nullptr, int img = -1) {
        return InsertItem(idx, s.c_str(), data, img);
    }

    int AddItem(const char *sz = "", void *data = nullptr, int img = -1) {
        return InsertItem(GetItemCount(), sz, data, img);
    }

    int AddItem(LPCWSTR sz, void *data = nullptr, int img = -1) {
        return InsertItem(GetItemCount(), sz, data, img);
    }

    int AddItem(const std::string &s, void *data = nullptr, int img = -1) {
        return InsertItem(GetItemCount(), s, data, img);
    }

    int AddItem(const std::wstring &s, void *data = nullptr, int img = -1) {
        return InsertItem(GetItemCount(), s, data, img);
    }

    void DeleteItem(int idx) {
        ListView_DeleteItem(m_hWnd, idx);
    }

    void DeleteAllItems() {
        ListView_DeleteAllItems(m_hWnd);
    }

    void SetImageList(HIMAGELIST hIL, int iImgListType = LVSIL_SMALL) {
        ListView_SetImageList(m_hWnd, hIL, LVSIL_SMALL);
    }

    int GetItemCount() {
        return ListView_GetItemCount(m_hWnd);
    }

    int GetColumnCount() {
        return (int) ::SendMessage(ListView_GetHeader(m_hWnd), HDM_GETITEMCOUNT, 0, 0);
    }

    void InsertColumn(int idx, const char *szName, int width, bool bCenter = false);

    void AddColumn(const char *szName, int width = -1, bool bCenter = false) {
        InsertColumn(GetColumnCount(), szName, width, bCenter);
    }

    int GetColumnWidth(int idx) {
        return ListView_GetColumnWidth(m_hWnd, idx);
    }

    void SetColumnWidth(int idx, int w) {
        ListView_SetColumnWidth(m_hWnd, idx, w);
    }

    void SetItemImage(int idx, int image) {
        LVITEM lvi = {0};
        lvi.iItem = idx;
        lvi.iImage = image;
        lvi.mask = LVIF_IMAGE;
        ListView_SetItem(m_hWnd, &lvi);
    }

    void SetItemState(int idx, UINT state, UINT mask) {
        ListView_SetItemState(m_hWnd, idx, state, mask);
    }

    UINT GetItemState(int idx, UINT mask) {
        return ListView_GetItemState(m_hWnd, idx, mask);
    }

    void SelectItem(int idx) {
        SetItemState(idx, SELECTED_FOCUSED, SELECTED_FOCUSED);
    }

    void UnselectItem(int idx) {
        SetItemState(idx, 0, SELECTED_FOCUSED);
    }

    int GetSelectedItem() {
        int cnt = GetItemCount();
        for (int i = 0; i < cnt; i++)
            if (GetItemState(i, SELECTED_FOCUSED) == SELECTED_FOCUSED)
                return i;
        return -1;
    }

    std::vector<int> GetSelectedItems() {
        std::vector<int> v;
        int cnt = GetItemCount();
        for (int i = 0; i < cnt; i++)
            if (GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED)
                v.push_back(i);
        return v;
    }

    int GetSelectedCount() {
        return ListView_GetSelectedCount(m_hWnd);
    }

    LPARAM GetItemParam(int idx) {
        LVITEM lvi;
        lvi.iItem = idx;
        lvi.mask = LVIF_PARAM;
        ListView_GetItem(m_hWnd, &lvi);
        return lvi.lParam;
    }

    void SetItemParam(int idx, LPARAM lp) {
        LVITEM lvi;
        lvi.iItem = idx;
        lvi.mask = LVIF_PARAM;
        lvi.lParam = lp;
        ListView_SetItem(m_hWnd, &lvi);
    }

    void SetBkColor(COLORREF clr) {
        ListView_SetBkColor(m_hWnd, clr);
    }

    void SetTextBkcolor(COLORREF clr) {
        ListView_SetTextBkColor(m_hWnd, clr);
    }

    void SetItemText(int idx, const std::string &s) {
        SetSubItemText(idx, 0, s);
    }

    void SetItemText(int idx, const std::wstring &ws) {
        SetSubItemText(idx, 0, ws);
    }

    void SetSubItemText(int idx, int subIdx, const std::string &s) {
        LVITEMA lvi{};
        lvi.iSubItem = subIdx;
        lvi.pszText = (char *) s.c_str();
        SendMessage(LVM_SETITEMTEXTA, (WPARAM) idx, (LPARAM) &lvi);
    }

    void SetSubItemText(int idx, int subIdx, const std::wstring &ws) {
        LVITEMW lvi{};
        lvi.iSubItem = subIdx;
        lvi.pszText = (wchar_t *) ws.c_str();
        SendMessage(LVM_SETITEMTEXTW, (WPARAM) idx, (LPARAM) &lvi);
    }

    std::string GetItemText(int idx) {
        return GetSubItemText(idx, 0);
    }

    std::string GetSubItemText(int idx, int subIdx) {
        strbuf sBuf(MAX_PATH);
        LVITEMA lvi{};
        lvi.iSubItem = subIdx;
        lvi.pszText = sBuf.buf();
        lvi.cchTextMax = sBuf.size();
        int len = SendMessage(LVM_GETITEMTEXTA, (WPARAM) idx, (LPARAM) &lvi);
        return sBuf.str(len); // ?
    }

    std::wstring GetItemTextW(int idx) {
        return GetSubItemTextW(idx, 0);
    }

    std::wstring GetSubItemTextW(int idx, int subIdx) {
        wstrbuf sBuf(MAX_PATH);
        LVITEMW lvi{};
        lvi.iSubItem = subIdx;
        lvi.pszText = sBuf.buf();
        lvi.cchTextMax = sBuf.size();
        int len = SendMessage(LVM_GETITEMTEXTW, (WPARAM) idx, (LPARAM) &lvi);
        return sBuf.str(len); // ?
    }

    void EnsureVisible(int idx, bool bPartialOK = true) {
        ListView_EnsureVisible(m_hWnd, idx, bPartialOK);
    }

    int getItemIndexAt(const CPoint &pt) {
        LVHITTESTINFO hti = {0};
        hti.pt = pt;
        //hti.flags = LVHT_ONITEM;
        return ListView_HitTest(m_hWnd, &hti);
    }

    int getItemIndexAtCursor() {
        CPoint pt = GetCursorPos();
        ScreenToClient(pt);
        return getItemIndexAt(pt);
    }

    void SetFullRowSelect() {
        ListView_SetExtendedListViewStyleEx(m_hWnd, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
    }

    void SetCheckboxes() {
        ListView_SetExtendedListViewStyleEx(m_hWnd, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    }

    void SetGridLines() {
        ListView_SetExtendedListViewStyleEx(m_hWnd, LVS_EX_GRIDLINES, LVS_EX_GRIDLINES);
    };

    void SetCheckState(int idx, bool b) {
        ListView_SetCheckState(m_hWnd, idx, b ? TRUE : FALSE);
    }

    bool GetCheckState(int idx) {
        return ListView_GetCheckState(m_hWnd, idx) == TRUE;
    }
};

class CTabCtrl : public CWindow {
public:
    bool Create(HWND hParent, DWORD dwStyleAdd = 0);

    int InsertItem(const std::string &sText, int iImage = -1);

    int InsertItem(const std::wstring &sText, int iImage = -1);

    void UpdateItem(int index, const std::string &sText, int iImage = -1);

    void UpdateItem(int index, const std::wstring &sText, int iImage = -1);

    void DeleteItem(int index) {
        TabCtrl_DeleteItem(m_hWnd, index);
    }

    int GetCurSel() {
        return TabCtrl_GetCurSel(m_hWnd);
    }

    void SetCurSel(int idx) {
        TabCtrl_SetCurSel(m_hWnd, idx);
    }

    int GetItemCount() {
        return TabCtrl_GetItemCount(m_hWnd);
    }

    CRect GetItemRect(int index) {
        CRect r;
        TabCtrl_GetItemRect(m_hWnd, index, &r);
        return r;
    }

    bool SetImageList(HIMAGELIST hIL) {
        return TabCtrl_SetImageList(m_hWnd, hIL) != 0;
    }

    HIMAGELIST GetImageList() {
        return TabCtrl_GetImageList(m_hWnd);
    }

    HWND GetToolTips() {
        return TabCtrl_GetToolTips(hWnd());
    }

    bool GetItem(int idx, TCITEMA *pItem) { return SendMessage(TCM_GETITEMA, (WPARAM) idx, (LPARAM) pItem) != 0; }

    bool GetItem(int idx, TCITEMW *pItem) { return SendMessage(TCM_GETITEMW, (WPARAM) idx, (LPARAM) pItem) != 0; }

    bool SetItem(int idx, TCITEMA *pItem) { return SendMessage(TCM_SETITEMA, (WPARAM) idx, (LPARAM) pItem) != 0; }

    bool SetItem(int idx, TCITEMW *pItem) { return SendMessage(TCM_SETITEMW, (WPARAM) idx, (LPARAM) pItem) != 0; }

    LPARAM GetItemParam(int idx) {
        TCITEM it{};
        it.mask = TCIF_PARAM;
        GetItem(idx, &it);
        return it.lParam;
    }

    void SetItemParam(int idx, LPARAM prm) {
        TCITEM it{};
        it.mask = TCIF_PARAM;
        it.lParam = prm;
        SetItem(idx, &it);
    }

    void SetPadding(int x, int y) {
        TabCtrl_SetPadding(m_hWnd, x, y);
    }

    int getItemIndexAt(const CPoint &pt) {
        TCHITTESTINFO hti = {0};
        hti.pt = pt;
        //hti.flags = TCHT_ONITEM;
        return TabCtrl_HitTest(m_hWnd, &hti);
    }

    int getItemIndexAtCursor() {
        CPoint pt = GetCursorPos();
        ScreenToClient(pt);
        return getItemIndexAt(pt);
    }
};

class CDialog : public CWindow {
protected:
    int m_IDD;

    virtual void onInit() = 0;

    virtual void onCommand(int cmd, int code) = 0;

    virtual void onDestroy() {}

    virtual void onNotify(LPARAM lParam) {}

    virtual BOOL onDrawClrBtn(DRAWITEMSTRUCT *pDIS) { return FALSE; }

public:
    bool doModal(HWND hParent) {
        return DialogBoxParam(g_hInst, MAKEINTRESOURCE(m_IDD), hParent, Proc, (LONG_PTR) this) != IDCANCEL;
    }

    static INT_PTR CALLBACK Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    INT_PTR ThisProc(UINT message, WPARAM wParam, LPARAM lParam);
};

class CHotKey : public CWindow {
public:
    void SetHotKey(int hotKey) {
        SendMessage(HKM_SETHOTKEY, hotKey);
    }

    void GetHotKey(int &hotKey) {
        hotKey = LOWORD(SendMessage(HKM_GETHOTKEY));
    }
};

/////////////////////////////////////////////////////////////////////

const unsigned WM_SPLITTER = WM_USER + 0x4001;

class CSplitter : public CWindow {
    bool m_bHoriz;
    bool m_bMove;
    int m_pos, m_pos1, m_pos2;
    HWND m_hParent;
    int m_w, m_h; // parent
public:
    CSplitter() : m_bHoriz(true), m_bMove(false) {
        m_pos = m_pos1 = m_pos2 = 0;
    }

    void setHoriz(int pos1, int pos2) {
        m_pos1 = pos1;
        m_pos2 = pos2;
    }

    bool Create(HWND hParent);

    static ATOM RegisterClass(HINSTANCE h);

private:
    static const char *szClassName;

    static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    LRESULT WndProc(UINT message, WPARAM wParam, LPARAM lParam);

    void onMouseDown(CPoint pt);

    void onMouseMove(CPoint pt);

    void onMouseUp();
};

/////////////////////////////////////////////////////////////////////
// menu
/////////////////////////////////////////////////////////////////////

class CMenu {
protected:
    HMENU m_hMenu;
    bool m_bDestroy, m_bPopup;
public:
    CMenu() {
        m_hMenu = nullptr;
        m_bDestroy = m_bPopup = false;
    }

    CMenu(HMENU hMenu) {
        Attach(hMenu);
    }

    CMenu(int id) {
        Load(id);
    }

    ~CMenu() {
        if (m_hMenu && m_bDestroy) ::DestroyMenu(m_hMenu);
    }

    void Create(bool bPopup = false) {
        m_hMenu = bPopup ? ::CreatePopupMenu() : ::CreateMenu();
        m_bDestroy = true;
        m_bPopup = bPopup;
    }

    void Load(int id, bool bPopup = false) {
        m_hMenu = ::LoadMenu(g_hInst, MAKEINTRESOURCE(id));
        m_bDestroy = true;
        m_bPopup = bPopup;
    }

    void Attach(HMENU hMenu, bool bPopup = false) {
        m_hMenu = hMenu;
        m_bDestroy = false;
        m_bPopup = bPopup;
    }

    HMENU hMenu() {
        return m_hMenu;
    }

    int TrackPopup(UINT flags, int x, int y, HWND hWnd) {
        HMENU hPopup = m_bPopup ? m_hMenu : GetSubMenu(m_hMenu, 0);
        return ::TrackPopupMenu(hPopup, flags, x, y, 0, hWnd, 0);
    }

    int TrackPopup(UINT flags, const CPoint &pt, HWND hWnd) {
        return TrackPopup(flags, pt.x, pt.y, hWnd);
    }

    int TrackPopup(UINT flags, HWND hWnd) {
        return TrackPopup(flags, GetCursorPos(), hWnd);
    }

    void SetPopupDefaultItem(int cmd) {
        SetMenuDefaultItem(GetSubMenu(m_hMenu, 0), cmd, 0);
    }

    void Enable(int cmd, bool b) {
        UINT flag = b ? MF_ENABLED : MF_DISABLED | MF_GRAYED;
        EnableMenuItem(m_hMenu, cmd, MF_BYCOMMAND | flag);
    }

    void Check(int cmd, bool b) {
        UINT flag = b ? MF_CHECKED : MF_UNCHECKED;
        CheckMenuItem(m_hMenu, cmd, MF_BYCOMMAND | flag);
    }

    void EnableCheck(int cmd, bool bEnable, bool bCheck) {
        Enable(cmd, bEnable);
        Check(cmd, bCheck);
    }

    bool isChecked(int cmd) {
        return GetMenuState(m_hMenu, cmd, MF_BYCOMMAND) == MF_CHECKED;
    }

    void Text(int cmd, const char *szText) {
        ::ModifyMenuA(m_hMenu, cmd, MF_BYCOMMAND | MF_STRING, cmd, szText);
    }

    void Text(int cmd, const std::string &sText) {
        Text(cmd, sText.c_str());
    }

    void Text(int cmd, LPCWSTR szText) {
        ::ModifyMenuW(m_hMenu, cmd, MF_BYCOMMAND | MF_STRING, cmd, szText);
    }

    void Text(int cmd, const std::wstring &sText) {
        Text(cmd, sText.c_str());
    }

    std::string Text(int cmd) { return GetMenuText(m_hMenu, cmd, MF_BYCOMMAND); }

    std::wstring TextW(int cmd) { return GetMenuTextW(m_hMenu, cmd, MF_BYCOMMAND); }

    void Append(int cmd, const char *szText, bool bEnabled = true) {
        ::AppendMenu(m_hMenu, MF_STRING | (bEnabled ? 0 : MF_DISABLED | MF_GRAYED), cmd, szText);
    }

    void Append(int cmd, LPCWSTR szText, bool bEnabled = true) {
        ::AppendMenuW(m_hMenu, MF_STRING | (bEnabled ? 0 : MF_DISABLED | MF_GRAYED), cmd, szText);
    }

    void Append(int cmd, const std::string &sText, bool bEnabled = true) {
        Append(cmd, sText.c_str(), bEnabled);
    }

    void Append(int cmd, const std::wstring &sText, bool bEnabled = true) {
        Append(cmd, sText.c_str(), bEnabled);
    }

    void AppendSeparator() {
        ::AppendMenu(m_hMenu, MF_SEPARATOR, 0, NULL);
    }

    void Delete(int cmd) {
        ::DeleteMenu(m_hMenu, cmd, MF_BYCOMMAND);
    }

    void DeleteByPos(int pos) {
        ::DeleteMenu(m_hMenu, pos, MF_BYPOSITION);
    }

    int GetItemCount() {
        return ::GetMenuItemCount(m_hMenu);
    }

    void ClearItems() {
        while (GetItemCount() > 0)
            DeleteByPos(0);
    }
};

/////////////////////////////////////////////////////////////////////
// registry
/////////////////////////////////////////////////////////////////////

class CRegistry {
    HKEY m_hKey;
    LONG m_ret;
public:
    CRegistry() {
        m_hKey = 0;
    }

    ~CRegistry() {
        Close();
    }

    bool Open(HKEY hKey, const char *szPath);

    bool Create(HKEY hKey, const char *szPath);

    void Close() {
        if (m_hKey) RegCloseKey(m_hKey);
        m_hKey = nullptr;
    }

    bool DeleteValue(const char *szValue) {
        return RegDeleteValue(m_hKey, szValue) == ERROR_SUCCESS;
    }

    static bool Delete(HKEY hKey, const char *szPath) {
        return RegDeleteKey(hKey, szPath) == ERROR_SUCCESS;
    }

    bool Open(HKEY hKey, const std::string &sPath) {
        return Open(hKey, sPath.c_str());
    }

    bool Create(HKEY hKey, const std::string &sPath) {
        return Create(hKey, sPath.c_str());
    }

    bool DeleteValue(const std::string &sValue) {
        return DeleteValue(sValue.c_str());
    }

    static bool Delete(HKEY hKey, const std::string &sPath) {
        return Delete(hKey, sPath.c_str());
    }

    int ReadInt(const char *szKey, int iDef = 0);

    std::string ReadStr(const char *szKey, const char *szDef = "");

    std::wstring ReadStrW(const char *sKey, LPCWSTR szDef = L"");

    void WriteInt(const char *szKey, int iValue);

    void WriteStr(const char *szKey, const char *szValue);

    void WriteStr(const char *szKey, LPCWSTR szValue);

    int ReadInt(const std::string &sKey, int iDef = 0) {
        return ReadInt(sKey.c_str(), iDef);
    }

    std::string ReadStr(const std::string &sKey, const std::string &sDef = "") {
        return ReadStr(sKey.c_str(), sDef.c_str());
    }

    std::wstring ReadStrW(const std::string &sKey, const std::wstring &sDef = L"") {
        return ReadStrW(sKey.c_str(), sDef.c_str());
    }

    void WriteInt(const std::string &sKey, int iValue) {
        WriteInt(sKey.c_str(), iValue);
    }

    void WriteStr(const std::string &sKey, const std::string &sValue) {
        WriteStr(sKey.c_str(), sValue.c_str());
    }

    void WriteStr(const std::string &sKey, const std::wstring &wsValue) {
        WriteStr(sKey.c_str(), wsValue.c_str());
    }

    int ret() const {
        return m_ret;
    }

    bool isErrNotFound() const {
        return m_ret == ERROR_FILE_NOT_FOUND;
    }

    bool isErrNoAccess() const {
        return m_ret == ERROR_ACCESS_DENIED;
    }
};

/////////////////////////////////////////////////////////////////////
// ini-file
/////////////////////////////////////////////////////////////////////

class CIniFile {
    std::string m_sFile, m_sSection;
public:
    CIniFile(const std::string &sFile) {
        m_sFile = sFile;
    }

    void useSection(const std::string &sSection) {
        m_sSection = sSection;
    }

    void enumKeys(const std::string &sSection, std::set<std::string> &st);

    void enumKeys(const std::string &sSection, std::vector<std::string> &v);

    void enumKeysValues(const std::string &sSection, std::map<std::string, std::string> &m);

    int ReadInt(const char *szKey, int iDef = 0) {
        return (int) GetPrivateProfileInt(m_sSection.c_str(), szKey, iDef, m_sFile.c_str());
    }

    std::string ReadStr(const char *szKey, const char *szDef = "") {
        strbuf sBuf(MAX_PATH);
        DWORD len = GetPrivateProfileStringA(m_sSection.c_str(), szKey, szDef, sBuf.buf(), sBuf.size(),
                                             m_sFile.c_str());
        return sBuf.str(len); // len without ending zero
    }

    void WriteInt(const char *szKey, int iValue) {
        WriteStr(szKey, itoa(iValue));
    }

    void WriteStr(const char *szKey, const char *szValue) {
        WritePrivateProfileStringA(m_sSection.c_str(), szKey, szValue, m_sFile.c_str());
    }

    int ReadInt(const std::string &sKey, int iDef = 0) {
        return ReadInt(sKey.c_str(), iDef);
    }

    std::string ReadStr(const std::string &sKey, const std::string &sDef = "") {
        return ReadStr(sKey.c_str(), sDef.c_str());
    }

    void WriteInt(const std::string &sKey, int iValue) {
        WriteInt(sKey.c_str(), iValue);
    }

    void WriteStr(const std::string &sKey, const std::string &sValue) {
        WriteStr(sKey.c_str(), sValue.c_str());
    }
};
///////////////////////////////////////////////////////////
// ACX
///////////////////////////////////////////////////////////

inline void ReadWindowXY(CRegistry &reg, CRect &r) {
    r.left = reg.ReadInt("x", r.left);
    r.top = reg.ReadInt("y", r.top);
}

inline void WriteWindowXY(CRegistry &reg, const CRect &r) {
    reg.WriteInt("x", r.left);
    reg.WriteInt("y", r.top);
}

inline bool ReadWindowRect(CRegistry &reg, CRect &r) {
    r.left = reg.ReadInt("x", r.left);
    r.top = reg.ReadInt("y", r.top);
    r.setWidth(reg.ReadInt("w", r.width()));
    r.setHeight(reg.ReadInt("h", r.height()));
    int maxX = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int maxY = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    return r.left < maxX && r.right > 0 && r.top < maxY && r.bottom > 0;
}

inline void WriteWindowRect(CRegistry &reg, const CRect &r) {
    reg.WriteInt("x", r.left);
    reg.WriteInt("y", r.top);
    reg.WriteInt("w", r.width());
    reg.WriteInt("h", r.height());
}

inline bool ReadWindowPlacement(CRegistry &reg, WINDOWPLACEMENT &wp) {
    wp.showCmd = reg.ReadInt("showCmd", wp.showCmd);
    wp.flags = reg.ReadInt("flags", wp.flags);
    wp.ptMinPosition.x = reg.ReadInt("min_x", wp.ptMinPosition.x);
    wp.ptMinPosition.y = reg.ReadInt("min_y", wp.ptMinPosition.y);
    wp.ptMaxPosition.x = reg.ReadInt("max_x", wp.ptMaxPosition.x);
    wp.ptMaxPosition.y = reg.ReadInt("max_y", wp.ptMaxPosition.y);
    wp.rcNormalPosition.left = reg.ReadInt("x", wp.rcNormalPosition.left);
    wp.rcNormalPosition.right = reg.ReadInt("y", wp.rcNormalPosition.right);
    wp.rcNormalPosition.top = reg.ReadInt("x2", wp.rcNormalPosition.top);
    wp.rcNormalPosition.bottom = reg.ReadInt("y2", wp.rcNormalPosition.bottom);
    int maxX = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int maxY = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (wp.ptMinPosition.x >= maxX
        || wp.ptMaxPosition.x >= maxX
        || wp.ptMinPosition.y >= maxY
        || wp.ptMaxPosition.y >= maxY
        || wp.rcNormalPosition.left >= maxX
        || wp.rcNormalPosition.right >= maxX
        || wp.rcNormalPosition.top >= maxY
        || wp.rcNormalPosition.bottom >= maxY)
        return false;
    return true;
}

inline void WriteWindowPlacement(CRegistry &reg, const WINDOWPLACEMENT &wp) {
    reg.WriteInt("showCmd", wp.showCmd);
    reg.WriteInt("flags", wp.flags);
    reg.WriteInt("min_x", wp.ptMinPosition.x);
    reg.WriteInt("min_y", wp.ptMinPosition.y);
    reg.WriteInt("max_x", wp.ptMaxPosition.x);
    reg.WriteInt("max_y", wp.ptMaxPosition.y);
    reg.WriteInt("x", wp.rcNormalPosition.left);
    reg.WriteInt("y", wp.rcNormalPosition.right);
    reg.WriteInt("x2", wp.rcNormalPosition.top);
    reg.WriteInt("y2", wp.rcNormalPosition.bottom);
}

///////////////////////////////////////////////////////////
// Load & Save

#define LEFTTOP_NONE    0
#define LEFTTOP_CENTER    1
#define LEFTTOP_RANDOM    2
#define WP_NO_MINIMIZE    16

inline void LoadWindowXY(HWND hWnd, const std::string &sSection) {
    CRect r;
    GetWindowRect(hWnd, &r);
    CRect r2 = r;
    CRegistry reg;
    if (reg.Open(g_hKey, g_szPath + sSection))
        ReadWindowXY(reg, r);
    MoveWindow(hWnd, r.left, r.top, r2.width(), r2.height(), FALSE);
}

inline void SaveWindowXY(HWND hWnd, const std::string &sSection) {
    CRect r;
    GetWindowRect(hWnd, &r);
    CRegistry reg;
    if (reg.Create(g_hKey, g_szPath + sSection))
        WriteWindowXY(reg, r);
}

inline void LoadWindowRect(HWND hWnd, const std::string &sSection, int w = 0, int h = 0, int mode = 0) {
    CRect r;
    GetWindowRect(hWnd, &r);
    CRegistry reg;
    if (!reg.Open(g_hKey, g_szPath + sSection) || !ReadWindowRect(reg, r)) {
        if (w) r.setWidth(w);
        if (h) r.setHeight(h);
        if (mode == LEFTTOP_CENTER) {
            CRect r2;
            HWND hParent = GetParent(hWnd);
            if (hParent)
                GetWindowRect(hParent, &r2);
            else
                r2 = CRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            r.setLeft(r2.left + (r2.width() - r.width()) / 2);
            r.setTop(r2.top + (r2.height() - r.height()) / 2);
        } else if (mode == LEFTTOP_RANDOM) {
            static int x = 0, y = 0;
            x += 100;
            y += 100;
            if (x + w / 2 > GetSystemMetrics(SM_CXSCREEN))
                x = 0;
            if (y + h / 2 > GetSystemMetrics(SM_CYSCREEN))
                y = 0;
            r.setLeft(x);
            r.setTop(y);
        }
    }
    MoveWindow(hWnd, r.left, r.top, r.width(), r.height(), FALSE);
}

inline void SaveWindowRect(HWND hWnd, const std::string &sSection) {
    CRect r;
    GetWindowRect(hWnd, &r);
    CRegistry reg;
    if (reg.Create(g_hKey, g_szPath + sSection))
        WriteWindowRect(reg, r);
}

inline void LoadWindowPlacement(HWND hWnd, const std::string &sSection, int w = 0, int h = 0, int mode = 0) {
    CWindowPlacement wp;
    GetWindowPlacement(hWnd, &wp);
    CRegistry reg;
    if (!reg.Open(g_hKey, g_szPath + sSection) || !ReadWindowPlacement(reg, wp)) {
        if (w) wp.setWidth(w);
        if (h) wp.setHeight(h);
        if (mode == LEFTTOP_CENTER) {
            CRect r2;
            HWND hParent = GetParent(hWnd);
            if (hParent)
                GetWindowRect(hParent, &r2);
            else
                r2 = CRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            wp.setLeft(r2.left + (r2.width() - wp.width()) / 2);
            wp.setTop(r2.top + (r2.height() - wp.height()) / 2);
        } else if (mode == LEFTTOP_RANDOM) {
            static int x = 0, y = 0;
            x += 100;
            y += 100;
            if (x + w / 2 > GetSystemMetrics(SM_CXSCREEN))
                x = 0;
            if (y + h / 2 > GetSystemMetrics(SM_CYSCREEN))
                y = 0;
            wp.setLeft(x);
            wp.setTop(y);
        }
    }
    if (mode & WP_NO_MINIMIZE)
        if (wp.showCmd == SW_SHOWMINIMIZED)
            wp.showCmd = SW_SHOWNORMAL;
    SetWindowPlacement(hWnd, &wp);
}

inline void SaveWindowPlacement(HWND hWnd, const std::string &sSection) {
    WINDOWPLACEMENT wp = {0};
    wp.length = sizeof(wp);
    GetWindowPlacement(hWnd, &wp);
    CRegistry reg;
    if (reg.Create(g_hKey, g_szPath + sSection))
        WriteWindowPlacement(reg, wp);
}

inline void LoadColumnWidths(CListCtrl &lv, const char *szSection) {
    CRegistry reg;
    if (reg.Open(g_hKey, std::string(g_szPath) + szSection)) {
        int cnt = lv.GetColumnCount();
        for (int i = 0; i < cnt; ++i) {
            int w = reg.ReadInt("column" + itoa(i), lv.GetColumnWidth(i));
            lv.SetColumnWidth(i, w);
        }
    }
}

inline void SaveColumnWidths(CListCtrl &lv, const char *szSection) {
    CRegistry reg;
    if (reg.Open(g_hKey, std::string(g_szPath) + szSection)) {
        int cnt = lv.GetColumnCount();
        for (int i = 0; i < cnt; ++i)
            reg.WriteInt("column" + itoa(i), lv.GetColumnWidth(i));
    }
}

///////////////////////////////////////////////////////

inline void LoadStr(const char *szSection, const char *szKey, std::string &str) {
    CRegistry reg;
    if (reg.Open(g_hKey, std::string(g_szPath) + szSection))
        str = reg.ReadStr(szKey, str);
}

inline void SaveStr(const char *szSection, const char *szKey, const std::string &str) {
    CRegistry reg;
    if (reg.Create(g_hKey, std::string(g_szPath) + szSection))
        reg.WriteStr(szKey, str);
}

inline void LoadInt(const std::string &sSection, const char *szKey, int &val) {
    CRegistry reg;
    if (reg.Open(g_hKey, g_szPath + sSection))
        val = reg.ReadInt(szKey, val);
}

inline int LoadIntVal(const std::string &sSection, const char *szKey, int val) {
    CRegistry reg;
    if (reg.Open(g_hKey, g_szPath + sSection))
        val = reg.ReadInt(szKey, val);
    return val;
}

inline void SaveInt(const std::string &sSection, const char *szKey, int val) {
    CRegistry reg;
    if (reg.Create(g_hKey, g_szPath + sSection))
        reg.WriteInt(szKey, val);
}

inline void LoadBool(const char *szSection, const char *szKey, bool &val) {
    CRegistry reg;
    if (reg.Open(g_hKey, std::string(g_szPath) + szSection))
        val = reg.ReadInt(szKey, val) != 0;
}

inline void SaveBool(const char *szSection, const char *szKey, bool val) {
    CRegistry reg;
    if (reg.Create(g_hKey, std::string(g_szPath) + szSection))
        reg.WriteInt(szKey, val ? 1 : 0);
}

///////////////////////////////////////////////////////
// var <-> ctrl

inline void acx(bool &b, CCheckBox &chk, bool bSave) {
    if (bSave)
        b = chk.IsChecked();
    else
        chk.SetCheck(b);
}

inline void acx(std::string &s, CEdit &ed, bool bSave) {
    if (bSave)
        s = ed.GetText();
    else
        ed.SetText(s);
}

inline void acx(int &i, CEdit &ed, bool bSave) {
    if (bSave)
        i = stoi(ed.GetText());
    else
        ed.SetText(itoa(i));
}

inline void acx(std::string &s, CComboBox &cmb, bool bSave) {
    if (bSave)
        s = cmb.GetText();
    else
        cmb.SetupCombo(s);
}

inline void acx(int &i, CComboBox &cmb, bool bSave) {
    if (bSave)
        i = stoi(cmb.GetText());
    else
        cmb.SetupCombo(itoa(i));
}

inline void acx_int(int &i, CComboBox &cmb, bool bSave) {
    if (bSave)
        i = cmb.GetComboInt();
    else
        cmb.SetupComboInt(i);
}

inline void acx_ex(LPARAM &i, CComboBox &cmb, bool bSave) {
    if (bSave)
        i = cmb.GetComboEx();
    else
        cmb.SetupComboEx(i);
}

inline void acx(int &hotKey, CHotKey &hk, bool bSave) {
    if (bSave)
        hk.GetHotKey(hotKey);
    else
        hk.SetHotKey(hotKey);
}

///////////////////////////////////////////////////////
// var <-> registry

inline void acx(const char *szSection, const char *szKey, std::string &str, bool bSave) {
    if (bSave)
        SaveStr(szSection, szKey, str);
    else
        LoadStr(szSection, szKey, str);
}

inline void acx(const char *szSection, const char *szKey, int &i, bool bSave) {
    if (bSave)
        SaveInt(szSection, szKey, i);
    else
        LoadInt(szSection, szKey, i);
}

inline void acx(const char *szSection, const char *szKey, bool &b, bool bSave) {
    if (bSave)
        SaveBool(szSection, szKey, b);
    else
        LoadBool(szSection, szKey, b);
}