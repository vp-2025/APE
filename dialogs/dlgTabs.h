#pragma once

#include "main.h"
#include "shared/win.h"

class CTabListView : public CListCtrl {
    WNDPROC defWndProc{};
public:
    bool Create(HWND hParent);
    static INT_PTR CALLBACK WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

class CTabsDlg : public CDialog {
    CTabListView list;
    const vector<CTabPage>& m_vTabs;
    const CShellImageList& m_ilTabs;
public:
    int iTab{};

    CTabsDlg(const vector<CTabPage>& vTabs, const CShellImageList& ilTabs)
        : m_vTabs(vTabs), m_ilTabs(ilTabs) { m_IDD=IDD_TABS; }
private:
    void onInit() override;
    void onCommand(int cmd, int code) override;
};