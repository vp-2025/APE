#include "stdafx.h"
#include "resource.h"
#include "dlgCheckList.h"
#include "lang.h"

#define ID_SELECTED    101

string SizeToStr(int64_t k) {
    return to_string(k);
}

string SizeSpan2Str(int64_t k) {
    string s;
    if( k > 0 ) s << '+' << k; else s << k;
    return s;
}

string TimeToStr(const CTime& tm) {
    return tm.Format("%d/%m/%Y %H:%M:%S");
}

string TimeSpan2Str(const CTimeSpan& ts) {
    int k = (int) ts.GetTotalSeconds();
    if( !k ) return ""; //"00:00:00";
    int sign = k;
    k = abs(k);
    int ss = k % 60;
    k /= 60;
    int mm = k % 60;
    k /= 60;
    int hh = k;
    string s;
    s << (sign > 0 ? '+' : '-') << Format("%i:%02i:%02i", hh, mm, ss);
    return s;
}

void CCheckListDlg::onInit() {
    list.Attach(m_hWnd, IDC_LIST);
    list.ModifyStyle(0, LVS_SHAREIMAGELISTS);
    list.SetImageList((HIMAGELIST) m_ilTabs);
    list.SetFullRowSelect();
    list.SetCheckboxes();

    if( bReload ) {
        list.AddColumn("File");
        list.AddColumn("Size");
        list.AddColumn("Time");
        for( const auto& tabPage: m_vTabs ) {
            if( tabPage.bNew )
                continue;
            if( !IsFileExists(tabPage.sFile) )
                continue; // todo : better handle for delete files
            CTime tm = vGetFileTime(tabPage.sFile);
            int64_t size = vGetFileSize(tabPage.sFile);
            if( tabPage.tm == tm && tabPage.size == size )
                continue;
            int idx = list.AddItem(utf2w(tabPage.sFile), (void*) &tabPage, m_ilTabs.getFileTypeIndex(tabPage.sFile));
            list.SetCheckState(idx, true);

            string sSizeDiff, sTimeDiff;
            if( size != tabPage.size )
                sSizeDiff = " (" + SizeSpan2Str(size - tabPage.size) + ")";
            if( tm != tabPage.tm )
                sTimeDiff = " (" + TimeSpan2Str(tm - tabPage.tm) + ")";

            list.SetSubItemText(idx, 1, SizeToStr(size) + sSizeDiff);
            list.SetSubItemText(idx, 2, TimeToStr(tm) + sTimeDiff);
        }
        SetText(list.GetItemCount() > 1 ? "Files were externally modified. Reload?"
                                        : "File was externally modified. Reload?");
        list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
        list.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
        list.SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);

        CButton btnCancel;
        btnCancel.Attach(m_hWnd, IDCANCEL);
        btnCancel.SetVisible(false);
    } else {
        SetWindowText(m_hWnd, "Save Changes?");
        list.AddColumn("File");

        for( const auto& tabPage: m_vTabs ) {
            if( tabPage.sci.isModified() ) {
                int idx = list.AddItem(utf2w(tabPage.sFile), (void*) &tabPage, m_ilTabs.getFileTypeIndex(tabPage.sFile));
                list.SetCheckState(idx, true);
            }
        }
        list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
    }

    CenterWindow(m_hWnd);
    g_lang.translateDlg(m_hWnd);
}

void CCheckListDlg::onCommand(int cmd, int ) {
    for( int i = 0; i < list.GetItemCount(); ++i ) {
        auto* pPage = (CTabPage*) list.GetItemParam(i);
        if( !pPage ) continue;
        bool bChecked = list.GetCheckState(i);
        if( bReload ) {
            if( cmd == IDYES && bChecked ) {
                pPage->loadFromFile();
                stChanged.insert(pPage->sFile);
            } else {
                pPage->reloadSizeAndTime();
            }
        } else {
            if( cmd == IDYES && bChecked ) {
                if( pPage->saveToFile() ) {
                    stChanged.insert(pPage->sFile);
                } else {
                    EndDialog(m_hWnd, IDCANCEL);
                    return;
                }
            }
        }
    }
    EndDialog(m_hWnd, cmd);
}
