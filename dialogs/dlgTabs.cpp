#include "stdafx.h"
#include "resource.h"
#include "dlgTabs.h"

#define ID_SELECTED    101

bool CTabListView::Create(HWND hParent) {
    int listViewStyles = WS_VISIBLE
                         | LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_NOSORTHEADER
                         | LVS_SINGLESEL | LVS_AUTOARRANGE | LVS_SHAREIMAGELISTS;
    if( !CListCtrl::Create(hParent, listViewStyles) )
        return false;

    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR) this);
    defWndProc = (WNDPROC) ::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR) CTabListView::WndProcStatic);

    auto exStyle = ListView_GetExtendedListViewStyle(m_hWnd);
    exStyle |= LVS_EX_FULLROWSELECT;
    ListView_SetExtendedListViewStyle(m_hWnd, exStyle);

    return true;
}

INT_PTR CALLBACK CTabListView::WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* pTab = (CTabListView*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if( pTab )
        return pTab->WndProc(hWnd, message, wParam, lParam);
    else
        return DefWindowProc(hWnd, message, wParam, lParam);
}

INT_PTR CTabListView::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch( message ) {
        case WM_KEYUP:
            if( wParam == VK_CONTROL )
                ::SendMessage(GetParent(), WM_COMMAND, ID_SELECTED, 0);
            return TRUE;

        case WM_MOUSEWHEEL : {
            short zDelta = (short) HIWORD(wParam);
            if( zDelta != 0 ) {
                int k = GetSelectedItem();
                UnselectItem(k);
                if( zDelta > 0 ) k--;
                if( zDelta < 0 ) k++;
                if( k < 0 ) k = GetItemCount() - 1;
                if( k >= GetItemCount() ) k = 0;
                SelectItem(k);
            }
            return TRUE;
        }

        case WM_KEYDOWN :
            return TRUE;

        case WM_GETDLGCODE : {
            MSG* msg = (MSG*) lParam;
            if( msg != NULL ) {
                if( (msg->message == WM_KEYDOWN) && (0x80 & GetKeyState(VK_CONTROL)) ) {
                    int d = 0;
                    if( ((msg->wParam == VK_TAB) && (0x80 & GetKeyState(VK_SHIFT))) || (msg->wParam == VK_UP) )
                        d--;
                    else if( (msg->wParam == VK_TAB) || (msg->wParam == VK_DOWN) )
                        d++;
                    if( d != 0 ) {
                        int k = GetSelectedItem();
                        UnselectItem(k);
                        k += d;
                        if( k < 0 ) k = GetItemCount() - 1;
                        if( k >= GetItemCount() ) k = 0;
                        SelectItem(k);
                    }
                } else {
                    return TRUE;
                }
            }
            return DLGC_WANTALLKEYS;
        }

        default :
            return ::CallWindowProc(defWndProc, hWnd, message, wParam, lParam);
    }
}

///////////////////////////////////////////////////////////

void CTabsDlg::onInit() {
    list.Create(m_hWnd);
    list.SetImageList((HIMAGELIST) m_ilTabs);
    list.InsertColumn(0, nullptr, 500);
    int maxwidth = -1;
    for(const auto& tab : m_vTabs) {
        string sFile = tab.sFile;
        if( tab.sci.isModified() )
			sFile << " *";
        list.InsertItem(list.GetItemCount(), utf2w(sFile).c_str(), nullptr, m_ilTabs.getFileTypeIndex(tab.sFile));
        maxwidth = max(maxwidth, ListView_GetStringWidth(list.hWnd(), utf2a(sFile).c_str())+20);
    }
    list.SelectItem(iTab);

    CRect rIco;
    ListView_GetItemRect(list.hWnd(), 0, &rIco, LVIR_ICON);
    int offs = 8;
    CRect r;
    r.setWidth(30 + rIco.width() + maxwidth + offs * 2);
    r.setHeight(10 + rIco.height() * (int)m_vTabs.size() + offs * 2);

    this->Move(r);
    CenterWindow(m_hWnd);
    list.Move(offs, offs, r.width() - offs * 2, r.height() - offs * 2);
    list.SetColumnWidth(0, r.width() - offs * 2);
}

void CTabsDlg::onCommand(int cmd, int code) {
    if( cmd == ID_SELECTED ) {
        iTab = list.GetSelectedItem();
        EndDialog(m_hWnd, IDOK);
    }
}