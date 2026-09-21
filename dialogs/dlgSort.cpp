#include "stdafx.h"
#include "resource.h"
#include "dlgSort.h"
#include "lang.h"
#include "shared/str.h"

void sortLines(CSciWrapper& sci, bool bAsc, bool bIgnoreCase, bool bTrim) {
    CWaitCursor cur;
    int sel1 = sci.GetSelectionStart();
    int sel2 = sci.GetSelectionEnd();
    int l1, l2, p1, p2;
    if( sel1 != sel2 ) {
        l1 = sci.LineFromPosition(sel1);
        l2 = sci.LineFromPosition(sel2);
        p1 = sci.GetLineStartPosition(l1);
        p2 = sci.GetLineStartPosition(l2 + 1);
    } else {
        l1 = 0;
        l2 = sci.GetLineCount() - 1;
        p1 = 0;
        p2 = sci.GetLength();
    }
    multimap<string, string> m;
    for( int i = l1; i <= l2; i++ ) {
        int k1 = sci.GetLineStartPosition(i);
        int k2 = sci.GetLineStartPosition(i + 1);
        string s = sci.GetTextRange(k1, k2);
        if( i == sci.GetLineCount() - 1 && !s.empty() && *s.rbegin() != '\r' && *s.rbegin() != '\n' )
            s += sci.GetEOLStr();
        string s1 = s;
        if( bIgnoreCase )
            toLower(s1);
        if( bTrim )
            trim(s1, true, false);
        m.insert(pair<string, string>(s1, s));
    }
    string s;
    if( bAsc )
        for( auto it = m.begin(); it != m.end(); it++ )
            s += it->second;
    else
        for( auto it = m.rbegin(); it != m.rend(); it++ )
            s += it->second;
    sci.BeginUndoAction();
    sci.SetTarget(p1, p2);
    sci.ReplaceTarget(s);
    sci.EndUndoAction();
    if( sel1 != sel2 )
        sci.SetSel(p1, p1 + s.length());
}

void CSortDlg::onInit() {
    chkAsc.Attach(m_hWnd, IDC_SORT_ASC);
    chkIgnoreCase.Attach(m_hWnd, IDC_SORT_IGNORECASE);
    chkTrim.Attach(m_hWnd, IDC_SORT_TRIM);

    chkAsc.SetCheck(true);

    CenterWindow(m_hWnd);
    g_lang.translateDlg(m_hWnd);
}

void CSortDlg::onCommand(int cmd, int code) {
    switch( cmd ) {
        case IDOK:
            sortLines(sci, chkAsc.IsChecked(), chkIgnoreCase.IsChecked(), chkTrim.IsChecked());
        case IDCANCEL:
            EndDialog(m_hWnd, cmd);
        default:
            return;
    }
}