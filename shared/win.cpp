#include "win.h"
#include <cassert>
#include <memory>
#include "str.h"

using namespace std;

#pragma comment(lib, "comctl32.lib")

void CenterWindow(HWND hWnd) {
    CRect r, rP;
    GetWindowRect(hWnd, &r);
    HWND hParent = GetParent(hWnd);
    if (hParent) {
        GetWindowRect(hParent, &rP);
    } else {
        rP.right = GetSystemMetrics(SM_CXSCREEN);
        rP.bottom = GetSystemMetrics(SM_CYSCREEN);
    }
    r.setLeft(rP.left + (rP.width() - r.width()) / 2);
    r.setTop(rP.top + (rP.height() - r.height()) / 2);
    MoveWindow(hWnd, r.left, r.top, r.width(), r.height(), TRUE);
}

void CenterWindowOnDesktop(HWND hWnd) {
    CRect r;
    GetWindowRect(hWnd, &r);
    r.setLeft((GetSystemMetrics(SM_CXSCREEN) - r.width()) / 2);
    r.setTop((GetSystemMetrics(SM_CYSCREEN) - r.height()) / 2);
    MoveWindow(hWnd, r.left, r.top, r.width(), r.height(), TRUE);
}

/////////////////////////////////////////////////////////////////////
// base classes
/////////////////////////////////////////////////////////////////////

int CWaitCursor::cnt;

void CFont::CreateFont(int weight) {
    LOGFONT lf = {};
    lf.lfWeight = weight;
    strcpy(lf.lfFaceName, "tahoma");
#ifdef __DEVICERESOLUTIONAWARE_H__
    lf.lfHeight = -DRA::SCALEX(12);
#else
    lf.lfHeight = -12;
#endif
    hFont = CreateFontIndirect(&lf);
}

/////////////////////////////////////////////////////////////////////
// ctrls
/////////////////////////////////////////////////////////////////////

void CWindow::ModifyStyle(int idx, DWORD dwRemove, DWORD dwAdd) {
    DWORD dwStyle = ::GetWindowLong(m_hWnd, idx);
    DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
    if (dwStyle != dwNewStyle)
        ::SetWindowLong(m_hWnd, idx, dwNewStyle);
}

/////////////////////////////////////////////////////////////////////
// CStatic

bool
CStatic::Create(HWND hParent, int x, int y, int w, int h, LPCTSTR szText, WORD id, DWORD dwStyle, DWORD dwStyleEx) {
    m_hWnd = CreateWindowEx(dwStyleEx, "STATIC", szText, dwStyle,
                            x, y, w, h, hParent, (HMENU) id, g_hInst, 0);
    return m_hWnd != 0;
}

/////////////////////////////////////////////////////////////////////
// CEdit

bool CEdit::Create(HWND hParent, int x, int y, int w, int h, LPCTSTR szText, WORD id, DWORD dwStyle, DWORD dwStyleEx) {
    m_hWnd = CreateWindowEx(dwStyleEx, "EDIT", szText, dwStyle,
                            x, y, w, h, hParent, (HMENU) id, g_hInst, 0);
    return m_hWnd != 0;
}

/////////////////////////////////////////////////////////////////////
// CComboBox

bool CComboBox::Create(HWND hParent, int x, int y, int w, int h, WORD id, DWORD dwStyle, DWORD dwStyleEx) {
    m_hWnd = CreateWindowEx(dwStyleEx, "COMBOBOX", "", dwStyle,
                            x, y, w, h, hParent, (HMENU) id, g_hInst, 0);
    return m_hWnd != 0;
}

string CComboBox::GetString(int idx) {
    int len = (int) SendMessage(CB_GETLBTEXTLEN, idx) + 1;
    strbuf sBuf(len); // len excluding ending zero
    len = SendMessageA(CB_GETLBTEXT, idx, (LPARAM) sBuf.buf());
    return sBuf.str(len); // len without ending zero
}

wstring CComboBox::GetStringW(int idx) {
    int len = (int) SendMessage(CB_GETLBTEXTLEN, idx) + 1;
    wstrbuf sBuf(len); // len excluding ending zero
    len = SendMessageW(CB_GETLBTEXT, idx, (LPARAM) sBuf.buf());
    return sBuf.str(len); // len without ending zero
}

void CComboBox::SetupComboEx(LPARAM id) {
    for (int i = 0; i < GetCount(); i++)
        if (GetItemData(i) == id) {
            SetCurSel(i);
            return;
        }
}

void CComboBox::UpdateMRU(int cnt) {
    wstring s = GetTextW();
    int idx = FindString(s);
    if (idx == 0) return;
    if (idx > 0)
        DeleteString(idx);
    InsertString(0, s);
    SetCurSel(0);
    while (GetCount() > cnt)
        DeleteString(GetCount() - 1);
}

void CComboBox::LoadCombo(const string &sSection) {
    Clear();
    CRegistry reg;
    if (!reg.Open(g_hKey, g_szPath + sSection))
        return;

    int cnt = reg.ReadInt("Count");
    for (int i = 0; i < cnt; i++)
        AddString(reg.ReadStrW("Item" + itoa(i)));
}

void CComboBox::SaveCombo(const string &sSection) {
    CRegistry reg;
    if (!reg.Create(g_hKey, g_szPath + sSection))
        return;

    int cnt = GetCount();
    reg.WriteInt("Count", cnt);
    for (int i = 0; i < cnt; i++)
        reg.WriteStr("Item" + itoa(i), GetStringW(i));
}


/////////////////////////////////////////////////////////////////////

void CCheckBox::GetCheck(int &is, int &b) {
    int k = (int) SendMessage(BM_GETCHECK);
    is = k == BST_INDETERMINATE ? 0 : 1;
    b = k == BST_CHECKED ? 1 : 0;
}

void CCheckBox::SetCheck(int is, int b) {
    int k = 0;
    if (!is)
        k = BST_INDETERMINATE;
    else if (b)
        k = BST_CHECKED;
    else
        k = BST_UNCHECKED;
    SendMessage(BM_SETCHECK, k);
}

/////////////////////////////////////////////////////////////////////

bool CButton::Create(HWND hParent, int x, int y, int w, int h, LPCTSTR szText, WORD id, DWORD dwStyle) {
    m_hWnd = CreateWindow("BUTTON", szText, dwStyle,
                          x, y, w, h, hParent, (HMENU) id, g_hInst, 0);
    return m_hWnd != 0;
}

/////////////////////////////////////////////////////////////////////

void InitCommonControls(DWORD dw) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = dw;
    InitCommonControlsEx(&icex);
}

bool CStatusbar::Create(HWND hParent, WORD id, DWORD dwStyle) {
    InitCommonControls(ICC_BAR_CLASSES);

    m_hWnd = CreateStatusWindow(dwStyle, NULL, hParent, id);
    return m_hWnd != 0;
}

/////////////////////////////////////////////////////////////////////

bool CToolbar::Create(HWND hParent, int resID, int cnt, TBBUTTON buttons[], WORD id, DWORD dwStyle) {
    InitCommonControls(ICC_BAR_CLASSES);

/*	float dpiC = 1.0f; //GetScaleDPI();
	HBITMAP hBitmap = (HBITMAP)LoadImage(g_hInst, MAKEINTRESOURCE(resID), IMAGE_BITMAP, 16*cnt*dpiC, 15*dpiC, LR_LOADTRANSPARENT);
	m_hWnd = CreateToolbarEx(
	             hParent,
	             dwStyle,
	             id,
	             1,          // Number of button images
	             0,
	             (UINT_PTR)hBitmap,
	             buttons,
	             cnt,
	             16*dpiC,	// Width of the button in pixels
	             15*dpiC,	// Height of the button in pixels
	             0,			// Button image width in pixels
	             0,			// Button image height in pixels
	             sizeof(TBBUTTON) );
	SendMessage(TB_AUTOSIZE);
	return m_hWnd!=0;*/
    m_hWnd = CreateToolbarEx(
            hParent,
            dwStyle,
            id,
            1,          // Number of button images
            g_hInst,
            resID,
            buttons,
            cnt,
            16,       // Width of the button in pixels
            16,       // Height of the button in pixels
            0,        // Button image width in pixels
            0,        // Button image height in pixels
            sizeof(TBBUTTON));
    return m_hWnd != 0;
}

/////////////////////////////////////////////////////////////////////

void CReBar::Create(HWND hParent) {
    InitCommonControls(ICC_COOL_CLASSES);

    m_hWnd = CreateWindowEx(WS_EX_TOOLWINDOW,
                            REBARCLASSNAME, NULL,
                            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_VARHEIGHT |
                            RBS_BANDBORDERS | CCS_NODIVIDER | CCS_NOPARENTALIGN,
                            0, 0, 0, 0, hParent, NULL, g_hInst, NULL);

    REBARINFO rbi = {0};
    rbi.cbSize = sizeof(REBARINFO);
    SendMessage(RB_SETBARINFO, 0, (LPARAM) &rbi);
}

void CReBar::addBand(CWindow &wnd) {
    REBARBANDINFO rbbi = {0};
    rbbi.cbSize = sizeof(rbbi);
    rbbi.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE;
    rbbi.fStyle = RBBS_USECHEVRON | RBBS_GRIPPERALWAYS;
    rbbi.hwndChild = wnd.hWnd();
    rbbi.cyMinChild = wnd.getHeight();
    SendMessage(RB_INSERTBAND, (WPARAM) -1, (LPARAM) &rbbi);
}

/////////////////////////////////////////////////////////////////////

bool CTreeCtrl::Create(HWND hParent, DWORD dwStyleAdd) {
    InitCommonControls(ICC_TREEVIEW_CLASSES);

    CRect r;
    ::GetClientRect(hParent, &r);
    m_hWnd = CreateWindow(WC_TREEVIEW, "", WS_CHILD | dwStyleAdd, 0, 0, r.width(), r.height(),
                          hParent, (HMENU) 0, g_hInst, NULL);
    return m_hWnd != NULL;
}

HTREEITEM CTreeCtrl::InsertItem(HTREEITEM hParent, const char *sz, void *data) {
    TVINSERTSTRUCT is = {0};
    is.hParent = hParent;
    is.hInsertAfter = 0;
    is.item.mask = TVIF_TEXT;
    is.item.pszText = (char *) sz;
    if (data) {
        is.item.mask |= TVIF_PARAM;
        is.item.lParam = (LPARAM) data;
    }
    return TreeView_InsertItem(m_hWnd, &is);
}

void *CTreeCtrl::GetItemData(HTREEITEM hItem) {
    TVITEM tvi = {0};
    tvi.hItem = hItem;
    tvi.mask = TVIF_PARAM;
    TreeView_GetItem(m_hWnd, &tvi);
    return (void *) tvi.lParam;
}

/////////////////////////////////////////////////////////////////////

bool CListCtrl::Create(HWND hParent, DWORD dwStyleAdd) {
    InitCommonControls(ICC_LISTVIEW_CLASSES);

    CRect r;
    ::GetClientRect(hParent, &r);
    m_hWnd = CreateWindow(WC_LISTVIEW, "", WS_CHILD | dwStyleAdd, 0, 0, r.width(), r.height(),
                          hParent, (HMENU) 0, g_hInst, NULL);
    return m_hWnd != NULL;
}

int CListCtrl::InsertItem(int idx, const char *sz, void *data, int img) {
    LVITEM li = {0};
    li.iItem = idx;
    li.iSubItem = 0;
    li.pszText = (char *) sz;
    li.mask = LVIF_TEXT;
    if (data) {
        li.mask |= LVIF_PARAM;
        li.lParam = (LPARAM) data;
    }
    if (img != -1) {
        li.mask |= LVIF_IMAGE;
        li.iImage = img;
    }
    return SendMessage(LVM_INSERTITEMA, 0, (LPARAM) &li);
}

int CListCtrl::InsertItem(int idx, const wchar_t *sz, void *data, int img) {
    LVITEMW li = {0};
    li.iItem = idx;
    li.iSubItem = 0;
    li.pszText = (wchar_t *) sz;
    li.mask = LVIF_TEXT;
    if (data) {
        li.mask |= LVIF_PARAM;
        li.lParam = (LPARAM) data;
    }
    if (img != -1) {
        li.mask |= LVIF_IMAGE;
        li.iImage = img;
    }
    return SendMessage(LVM_INSERTITEMW, 0, (LPARAM) &li);
}

void CListCtrl::InsertColumn(int idx, LPCSTR szName, int width, bool bCenter) {
    LVCOLUMN lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.pszText = (char *) szName;
    lvc.cx = width;
    if (bCenter) {
        lvc.mask |= LVCF_FMT;
        lvc.fmt = LVCFMT_CENTER;
    }
    ListView_InsertColumn(m_hWnd, idx, &lvc);
}

/////////////////////////////////////////////////////////////////////

bool CTabCtrl::Create(HWND hParent, DWORD dwStyleAdd) {
    InitCommonControls(ICC_TAB_CLASSES);

    CRect r;
    ::GetClientRect(hParent, &r);
    m_hWnd = CreateWindow(WC_TABCONTROL, "", WS_CHILD | dwStyleAdd, 0, 0, r.width(), r.height(),
                          hParent, (HMENU) 0, g_hInst, NULL);
    return m_hWnd != NULL;
}

int CTabCtrl::InsertItem(const string &sText, int iImage) {
    TCITEMA ti{};
    ti.mask = TCIF_TEXT | TCIF_IMAGE;
    ti.pszText = (char *) sText.c_str();
    ti.iImage = iImage;
    return SendMessage(TCM_INSERTITEMA, (WPARAM) TabCtrl_GetItemCount(m_hWnd), (LPARAM) &ti);
}

int CTabCtrl::InsertItem(const wstring &sText, int iImage) {
    TCITEMW ti{};
    ti.mask = TCIF_TEXT | TCIF_IMAGE;
    ti.pszText = (wchar_t *) sText.c_str();
    ti.iImage = iImage;
    return SendMessage(TCM_INSERTITEMW, (WPARAM) TabCtrl_GetItemCount(m_hWnd), (LPARAM) &ti);
}

void CTabCtrl::UpdateItem(int index, const string &sText, int iImage) {
    TCITEMA ti{};
    ti.mask = TCIF_TEXT | TCIF_IMAGE;
    ti.pszText = (char *) sText.c_str();
    ti.iImage = iImage;
    SetItem(index, &ti);
}

void CTabCtrl::UpdateItem(int index, const wstring &sText, int iImage) {
    TCITEMW ti{};
    ti.mask = TCIF_TEXT | TCIF_IMAGE;
    ti.pszText = (wchar_t *) sText.c_str();
    ti.iImage = iImage;
    SetItem(index, &ti);
}

/////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK CDialog::Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    CDialog *pDlg;
    if (message == WM_INITDIALOG) {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
        pDlg = (CDialog *) lParam;
        pDlg->m_hWnd = hWnd;
    } else
        pDlg = (CDialog *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    return pDlg ? pDlg->ThisProc(message, wParam, lParam) : FALSE;
}

INT_PTR CDialog::ThisProc(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG:
            onInit();
            return TRUE;// set or not focus to control

        case WM_DESTROY:
            onDestroy();
            return FALSE; // If an application processes this message, it should return zero.

        case WM_CLOSE:
            onCommand(IDCANCEL, 0);
            return FALSE; // If an application processes this message, it should return zero.

        case WM_COMMAND:
            onCommand(LOWORD(wParam), HIWORD(wParam));
            return FALSE; // If an application processes this message, it should return zero.

        case WM_DRAWITEM: // If an application processes this message, it should return TRUE.
            return onDrawClrBtn((DRAWITEMSTRUCT *) lParam);

        case WM_NOTIFY:
            onNotify(lParam);
            return FALSE; // The return value is ignored except for notification messages that specify otherwise.

        default:
            return FALSE; // DefDlgProc(m_hWnd, message, wParam, lParam);
    }
}
/////////////////////////////////////////////////////////////////////

const char *CSplitter::szClassName = "dgSplitter";

ATOM CSplitter::RegisterClass(HINSTANCE hInst) {
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = CSplitter::WndProcStatic;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInst;
    wcex.hIcon = 0;
    wcex.hIconSm = 0;
    wcex.hCursor = LoadCursor(NULL, IDC_SIZENS); // horiz or vert?
    wcex.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
    wcex.lpszMenuName = 0;
    wcex.lpszClassName = szClassName;
    return RegisterClassEx(&wcex);
}

LRESULT CALLBACK CSplitter::WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto *pThis = (CSplitter *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (pThis)
        return pThis->WndProc(message, wParam, lParam);
    else
        return DefWindowProc(hWnd, message, wParam, lParam);
}

bool CSplitter::Create(HWND hParent) {
    m_hParent = hParent;
    m_hWnd = CreateWindow(szClassName, nullptr, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hParent, 0, g_hInst, 0);
    if (m_hWnd)
        SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR) this);
    return m_hWnd != nullptr;
}

class CUpdateCanvas {
    HWND hWnd;
    HDC hDC;
    HPEN hPen, hPenOld;
public:
    CUpdateCanvas(HWND _hWnd) {
        hWnd = _hWnd;
        hDC = ::GetDC(hWnd);
        hPen = ::CreatePen(PS_SOLID, 4, RGB(0x80, 0x80, 0x80));
        hPenOld = (HPEN) ::SelectObject(hDC, hPen);
        ::SetROP2(hDC, R2_NOTXORPEN);
    }

    ~CUpdateCanvas() {
        ::SelectObject(hDC, hPenOld);
        ::ReleaseDC(hWnd, hDC);
    }

    void line(int x1, int y1, int x2, int y2) {
        ::MoveToEx(hDC, x1, y1, 0);
        ::LineTo(hDC, x2, y2);
    }
};

void CSplitter::onMouseDown(CPoint pt) {
    ClientToScreen(pt);
    ::ScreenToClient(m_hParent, &pt);

    CRect r;
    ::GetClientRect(m_hParent, &r);
    m_h = r.height();
    m_w = r.width();

    SetCapture(m_hWnd);
    m_bMove = true;
    m_pos = pt.y;
    CUpdateCanvas canvas(m_hParent);
    if (m_bHoriz)
        canvas.line(0, m_pos, m_w, m_pos);
}

void CSplitter::onMouseMove(CPoint pt) {
    ClientToScreen(pt);
    ::ScreenToClient(m_hParent, &pt);

    CUpdateCanvas canvas(m_hParent);
    if (m_bHoriz)
        canvas.line(0, m_pos, m_w, m_pos);
    m_pos = pt.y;
    if (m_pos1 && m_pos < m_pos1)
        m_pos = m_pos1;
    if (m_pos2 && m_pos > m_h - m_pos2)
        m_pos = m_h - m_pos2;
    if (m_bHoriz)
        canvas.line(0, m_pos, m_w, m_pos);
}

void CSplitter::onMouseUp() {
    m_bMove = false;
    ReleaseCapture();
    CUpdateCanvas canvas(m_hParent);
    canvas.line(0, m_pos, m_w, m_pos);
    ::PostMessage(m_hParent, WM_SPLITTER, m_pos, 0);
}

LRESULT CSplitter::WndProc(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_LBUTTONDOWN:
            onMouseDown(CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
            return 0;
        case WM_MOUSEMOVE:
            if (wParam & MK_LBUTTON && m_bMove)
                onMouseMove(CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
            return 0;
        case WM_LBUTTONUP:
            onMouseUp();
            return 0;
        default:
            return DefWindowProc(m_hWnd, message, wParam, lParam);
    }
}

/////////////////////////////////////////////////////////////////////
// registry
/////////////////////////////////////////////////////////////////////

bool CRegistry::Open(HKEY hKey, const char *szPath) {
    assert(!m_hKey);
//	m_ret = RegOpenKeyEx( hKey, szPath, 0,KEY_ALL_ACCESS, &m_hKey );
    m_ret = RegOpenKey(hKey, szPath, &m_hKey);
    return m_ret == ERROR_SUCCESS;
}

bool CRegistry::Create(HKEY hKey, const char *szPath) {
    assert(!m_hKey);
//	DWORD dwDisp;
//	m_ret = RegCreateKeyEx( hKey, szPath, 0, NULL, 0, KEY_ALL_ACCESS, 0, &m_hKey, &dwDisp );
    m_ret = RegCreateKey(hKey, szPath, &m_hKey);
    return m_ret == ERROR_SUCCESS;
}

int CRegistry::ReadInt(LPCSTR szValue, int iDef) {
    assert(m_hKey);
    DWORD dw, dwType, dwSize = sizeof(dw);
    m_ret = RegQueryValueEx(m_hKey, szValue, 0, &dwType, (BYTE *) &dw, &dwSize);
    bool b = m_ret == ERROR_SUCCESS && dwType == REG_DWORD;
    return b ? dw : iDef;
}

string CRegistry::ReadStr(LPCSTR szValue, LPCSTR szDef) {
    assert(m_hKey);
    DWORD dwType, dwSize = 0;
    RegQueryValueExA(m_hKey, szValue, nullptr, nullptr, nullptr, &dwSize);
    strbuf sBuf(dwSize); // size of the buffer  in bytes
    m_ret = RegQueryValueExA(m_hKey, szValue, nullptr, &dwType, (BYTE *) sBuf.buf(), &dwSize);
    bool b = m_ret == ERROR_SUCCESS && dwType == REG_SZ;
    return b ? sBuf.str() : szDef; // ?
}

wstring CRegistry::ReadStrW(LPCSTR szValue, LPCWSTR szDef) {
    assert(m_hKey);
    DWORD dwType, dwSize = 0;
    RegQueryValueExW(m_hKey, a2w(szValue).c_str(), nullptr, nullptr, nullptr, &dwSize);
    wstrbuf sBuf(dwSize / sizeof(wchar_t)); // size of the buffer  in bytes
    m_ret = RegQueryValueExW(m_hKey, a2w(szValue).c_str(), nullptr, &dwType, (BYTE *) sBuf.buf(), &dwSize);
    bool b = m_ret == ERROR_SUCCESS && dwType == REG_SZ;
    return b ? sBuf.str() : szDef; // ?
}

void CRegistry::WriteInt(LPCSTR szValue, int iVal) {
    assert(m_hKey);
    DWORD dw = iVal;
    m_ret = RegSetValueEx(m_hKey, szValue, 0, REG_DWORD, (const BYTE *) &dw, sizeof(dw));
}

void CRegistry::WriteStr(LPCSTR szValue, LPCSTR szVal) {
    assert(m_hKey);
    m_ret = RegSetValueEx(m_hKey, szValue, 0, REG_SZ, (const BYTE *) szVal, (int) strlen(szVal) + 1);
}

void CRegistry::WriteStr(LPCSTR szValue, LPCWSTR szVal) {
    assert(m_hKey);
    m_ret = RegSetValueExW(m_hKey, a2w(szValue).c_str(), 0, REG_SZ, (const BYTE *) szVal,
                           (int) (wcslen(szVal) + 1) * sizeof(wchar_t));
}

void CIniFile::enumKeys(const string &sSection, set <string> &st) {
    strbuf sBuf(32767);
    int len = (int) GetPrivateProfileSectionA(sSection.c_str(), sBuf.buf(), sBuf.size(), m_sFile.c_str());
    vector <string> v;
    split(sBuf.str(len), v, '\0'); // len without ending zero
    for (const string &z: v) {
        vector <string> w;
        split(z, w, '=');
        if (!w.empty())
            st.insert(w[0]);
    }
}

void CIniFile::enumKeys(const string &sSection, vector <string> &vec) {
    strbuf sBuf(32767);
    int len = (int) GetPrivateProfileSectionA(sSection.c_str(), sBuf.buf(), sBuf.size(), m_sFile.c_str());
    vector <string> v;
    split(sBuf.str(len), v, '\0'); // len without ending zero
    for (const string &z: v) {
        vector <string> w;
        split(z, w, '=');
        if (!w.empty())
            vec.push_back(w[0]);
    }
}

void CIniFile::enumKeysValues(const string &sSection, map <string, string> &m) {
    strbuf sBuf(32767);
    int len = (int) GetPrivateProfileSectionA(sSection.c_str(), sBuf.buf(), sBuf.size(), m_sFile.c_str());
    vector <string> v;
    split(sBuf.str(len), v, '\0'); // len without ending zero
    for (const string &z: v) {
        vector <string> w;
        split(z, w, '=');
        if (w.size() < 2) continue;
        m[w[0]] = w[1];
    }
}