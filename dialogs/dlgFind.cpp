#include "stdafx.h"
#include "resource.h"
#include "dlgFind.h"
#include "lang.h"
#include "apeEditor.h"
#include "ShellContextMenu.h"
#include "fileMask.h"
#include "shared/platform.h"
#include "theme.h"

CREOpt findOptsExt[] = {
    {"Cr",  "\\r"},
    {"Lf",  "\\n"},
    {"Tab", "\\t"},
    {nullptr}
};

CREOpt* replaceOptsExt = findOptsExt;

// the bits for CREOpt::modes, by m_mode
#define SCI_RE (1 << 2) // Scintilla regex with SCFIND_POSIX
#define STD_RE (1 << 3) // SCFIND_CXX11REGEX: ECMAScript

CREOpt findOptsRE[] = {
    {"Any Character",           "."},
    {"Character in Range",      "[ ]"},
    {"Character not in Range",  "[^ ]"},
    {"Digit",                   "\\d"},
    {"Space",                   "\\s"},
    {"Word Character",          "\\w"},
    {"-"},
    {"Start of Line",           "^"},
    {"End of Line",             "$"},
    {"Start of Word",           "\\<",   SCI_RE},
    {"End of Word",             "\\>",   SCI_RE},
    {"Word Boundary",           "\\b",   STD_RE}, // Scintilla regex: \b is a backspace char
    {"-"},
    {"0 or 1 Matches",          "?"},
    {"0 or more Matches",       "*"},
    {"1 or more Matches",       "+"},
    {"0 or more, lazy",         "*?"},
    {"n to m Matches",          "{ , }", STD_RE},
    {"-"},
    {"Tagged Region",           "( )"},
    {"Tagged Expression 1",     "\\1"},
    {"Alternative",             "|",     STD_RE},
    {"Followed by",             "(?= )", STD_RE},
    {"Not Followed by",         "(?! )", STD_RE},
    {nullptr}
};

// SCI_REPLACETARGETRE: the same for SCI_RE and STD_RE (BuiltinRegex::SubstituteByPosition)
CREOpt replaceOptsRE[] = {
    {"Whole Matching Text", "\\0"},
    {"-"},
    {"Tagged Expression 1", "\\1"},
    {"Tagged Expression 2", "\\2"},
    {"Tagged Expression 3", "\\3"},
    {"Tagged Expression 4", "\\4"},
    {"Tagged Expression 5", "\\5"},
    {"Tagged Expression 6", "\\6"},
    {"Tagged Expression 7", "\\7"},
    {"Tagged Expression 8", "\\8"},
    {"Tagged Expression 9", "\\9"},
    {"-"},
    {"Cr",                  "\\r"},
    {"Lf",                  "\\n"},
    {"Tab",                 "\\t"},
    {"Backslash",           "\\\\"},
    {nullptr}
};


void InsertText(HWND hEdit, const char* sz) {
    while( *sz )
        SendMessage(hEdit, WM_CHAR, *sz++, 0);
}

void CFindDlg::popupOptRE(CREOpt* opts, int mode, HWND hDlg, HWND hBtn, HWND hEdit) {
    CMenu popup;
    popup.Create(true);
    bool bSep = true; // no separator at the top, nor two together
    for( int i = 0; opts[i].szTitle; i++ ) {
        if( opts[i].modes && !(opts[i].modes & (1 << mode)) )
            continue;
        if( !strcmp(opts[i].szTitle, "-") ) {
            if( !bSep )
                popup.AppendSeparator();
            bSep = true;
            continue;
        }
        string sText = opts[i].szText;
        replaceStr(sText,"&","&&");
        popup.Append(i + 1, string(opts[i].szTitle) + '\t' + sText);
        bSep = false;
    }
    CRect r;
    GetWindowRect(hBtn, &r);
    int ret = popup.TrackPopup(TPM_RETURNCMD, r.left, r.bottom, hDlg);
    if( !ret )
        return;

    InsertText(hEdit, opts[ret - 1].szText);
}

void extended2string(string& s) {
    replaceStr(s, "\\r", "\r");
    replaceStr(s, "\\n", "\n");
    replaceStr(s, "\\t", "\t");
}

/////////////////////////////////////////////////////////////////////

bool CFindDlg::isDlgMsg(MSG& msg) {
    if( m_hWnd && IsWindow(m_hWnd) )
        return IsDialogMessage(m_hWnd, &msg) != 0;
    return false;
}

void CFindDlg::Find(bool bReplace, bool bInFiles) {
    m_pRerun = nullptr;
    if( !pEditor )
        return;
    CSciWrapper* pSci = pEditor->GetScintilla();
    if( !pSci && !bInFiles )
        return;

    if( !m_hWnd || !IsWindow(m_hWnd) )
        m_hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_FIND), pEditor->hWnd(), CFindDlg::Proc, (LPARAM) this);
    CenterWindow(m_hWnd);
    ShowWindow(SW_SHOW);

    bFind = false;
    if( pSci ) {
        string sSearch;
        if( pSci->hasSelection(100) )
            sSearch = pSci->GetSelText();
        else
            sSearch = pSci->GetWordFromPosition(pSci->GetCurrentPos());
        cmbSearch.SetCurSel(-1);
        if( pSci->isEncodingUtf() )
            cmbSearch.SetText(utf2w(sSearch));
        else
            cmbSearch.SetText(sSearch);
        pSci->ClearTarget();
    }
    chkReplace.SetCheck(bReplace);
    chkInFiles.SetCheck(bInFiles);
    ::SetFocus(cmbSearch.hWnd());

    // the folder of the current file, if no folder is given
    CTabPage* pPage = pEditor->GetCurPage();
    if( bInFiles && cmbPath.GetTextW().empty() && pPage && !pPage->bNew )
        cmbPath.SetText(utf2w(ExtractFilePath(pPage->sFile)));

    UpdateControls(true);
}

void CFindDlg::Rerun(CFindResults* pResults) {
    const CFindParams& params = pResults->m_params;
    Find(params.bReplace, params.bInFiles);
    if( !m_hWnd || !IsWindowVisible(m_hWnd) ) // no file to search in
        return;
    m_pRerun = pResults;
    cmbSearch.SetCurSel(-1);
    cmbSearch.SetText(params.wsFind);
    cmbReplace.SetText(params.wsReplace);
    cmbFile.SetText(params.wsFiles);
    cmbPath.SetText(params.wsPath);
    chkSubFolders.SetCheck(params.bSubFolders);
    setOptions(params.opts);
    UpdateControls();
    bool bSelection = params.bSelection && rbtnSelection.IsEnabled();
    rbtnSelection.SetCheck(bSelection);
    rbtnWhole.SetCheck(!bSelection);
}

void CFindDlg::FindNext(bool bNext) {
    if( !m_hWnd ) return;
    rbtnUp.SetCheck(!bNext);
    rbtnDown.SetCheck(bNext);
    onFind();
}

void CFindDlg::ClearIndicators() {
    CSciWrapper* pSci = pEditor->GetScintilla();
    if( !pSci )
        return;
    pSci->clearIndicator(INDIC_FIND);
    pSci->clearIndicator(INDIC_REPLACE);
}

/////////////////////////////////////////////////////////////////////

#define FIND_DLG "find"
#define FIND_CMB "find\\findCmb"
#define REPLACE_CMB "find\\replaceCmb"
#define FILE_CMB "find\\fileCmb"
#define PATH_CMB "find\\pathCmb"

void CFindDlg::onInit() {
    cmbSearch.Attach(m_hWnd, IDC_FIND_SEARCHTEXT); // load combo
    btnSearchOpt.Attach(m_hWnd, IDC_FIND_OPTION_FIND);

    chkReplace.Attach(m_hWnd, IDC_FIND_REPLACEON);
    cmbReplace.Attach(m_hWnd, IDC_FIND_REPLACETEXT); // load combo
    btnReplaceOpt.Attach(m_hWnd, IDC_FIND_FILE_REPLACE);

    chkInFiles.Attach(m_hWnd, IDC_FIND_FILES);
    cmbFile.Attach(m_hWnd, IDC_FIND_FILE_TYPES);
    cmbPath.Attach(m_hWnd, IDC_FIND_FILE_DIR);
    btnBrowse.Attach(m_hWnd, IDC_FIND_FILE_BRW);

    chkWholeWord.Attach(m_hWnd, IDC_FIND_MATCHWORDS);
    chkMatchCase.Attach(m_hWnd, IDC_FIND_MATCHCASE);

    btnMode.Attach(m_hWnd, IDC_FIND_MODE);

    chkSubFolders.Attach(m_hWnd, IDC_FIND_FILE_SUBDIRS);
    chkNewTab.Attach(m_hWnd, IDC_FIND_FILE_OUT2);
    chkCreateBak.Attach(m_hWnd, IDC_FIND_FILE_BACKUP);

    rbtnUp.Attach(m_hWnd, IDC_FIND_UP);
    rbtnDown.Attach(m_hWnd, IDC_FIND_DOWN);

    rbtnWhole.Attach(m_hWnd, IDC_FIND_REPLACEFILE);
    rbtnSelection.Attach(m_hWnd, IDC_FIND_REPLACESEL);

    btnSearch.Attach(m_hWnd, IDC_FIND_BTN);
    btnSearchAll.Attach(m_hWnd, IDC_FINDALL_BTN);
    btnReplace.Attach(m_hWnd, IDC_FIND_REPLACE_BTN);
    btnReplaceAll.Attach(m_hWnd, IDC_FIND_REPLACEALL_BTN);

    CenterWindow(m_hWnd);
    g_lang.translateDlg(m_hWnd);

    // Cc, W and the mode are toggle buttons: their meaning is in the tooltips
    HWND hTip = m_hTip = CreateWindowExW(0, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP, 0, 0, 0, 0,
                                m_hWnd, nullptr, g_hInst, nullptr);
    for( auto [id, szTip] : {pair{IDC_FIND_MATCHCASE, L"Match case"},
							 pair{IDC_FIND_MATCHWORDS, L"Match whole words"},
                             pair{IDC_FIND_MODE, L"Normal"}} ) {
        wstring wsTip = tr(wstring(szTip));
        TOOLINFOW ti = {sizeof(ti)};
        ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        ti.hwnd = m_hWnd;
        ti.uId = (UINT_PTR) GetDlgItem(m_hWnd, id);
        ti.lpszText = (LPWSTR) wsTip.c_str();
        ::SendMessageW(hTip, TTM_ADDTOOLW, 0, (LPARAM) &ti);
    }
    g_theme.applyCtrl(hTip);
    setMode(0);

    LoadWindowXY(m_hWnd, FIND_DLG);
    cmbSearch.LoadCombo(FIND_CMB);
    cmbReplace.LoadCombo(REPLACE_CMB);
    cmbFile.LoadCombo(FILE_CMB);
    cmbPath.LoadCombo(PATH_CMB);
    bool bNewTab = false;
    LoadBool(FIND_DLG, "NewTab", bNewTab);
    chkNewTab.SetCheck(bNewTab);
}

void CFindDlg::onDestroy() {
    SaveWindowXY(m_hWnd, FIND_DLG);
    cmbSearch.SaveCombo(FIND_CMB);
    cmbReplace.SaveCombo(REPLACE_CMB);
    cmbFile.SaveCombo(FILE_CMB);
    cmbPath.SaveCombo(PATH_CMB);
    SaveBool(FIND_DLG, "NewTab", chkNewTab.IsChecked());
}

void CFindDlg::onCommand(int cmd, int code) {
    switch( cmd ) {
        case IDCANCEL:
            ShowWindow(SW_HIDE);
            break;
        case IDC_FIND_OPTION_FIND:
            if( code == 0 ) {
                CREOpt* opts = nullptr;
                if( isModeExt() ) opts = findOptsExt;
                if( isModeRE() ) opts = findOptsRE;
                if( opts )
                    popupOptRE(opts, m_mode, m_hWnd, btnSearchOpt.hWnd(), cmbSearch.hWnd());
                UpdateControls();
            }
            break;
        case IDC_FIND_FILE_REPLACE:
            if( code == 0 ) {
                CREOpt* opts = nullptr;
                if( isModeExt() ) opts = replaceOptsExt;
                if( isModeRE() ) opts = replaceOptsRE;
                if( opts )
                    popupOptRE(opts, m_mode, m_hWnd, btnReplaceOpt.hWnd(), cmbReplace.hWnd());
                UpdateControls();
            }
            break;
        case IDC_FIND_FILE_BRW: {
            string s = cmbPath.GetText();
            if( BrowseForFolder(m_hWnd, s) )
                cmbPath.SetText(s);
        }
            break;
        case IDC_FIND_BTN:
        case IDC_FINDALL_BTN:
            if( chkInFiles.IsChecked() )
                onFindInFiles();
            else
            if( cmd == IDC_FIND_BTN )
                onFind();
            else
                onFindAll();
            break;
        case IDC_FIND_REPLACE_BTN:
            onReplace();
            break;
        case IDC_FIND_REPLACEALL_BTN:
            onReplaceAll();
            break;
        case IDC_FIND_MODE:
            if( code == BN_CLICKED )
                popupMode();
            UpdateControls();
            break;
        case IDC_FIND_SEARCHTEXT:
            if( code == CBN_SELCHANGE ) // a string from the history: its options
                setOptions(cmbSearch.GetItemData(cmbSearch.GetCurSel()));
            UpdateControls();
            break;
        default:
            UpdateControls();
    }
}

void CFindDlg::UpdateControls(bool bPrepare) {
    CSciWrapper* pSci = pEditor->GetScintilla();
    chkInFiles.Enable(pSci != nullptr); // without an editor only in files
    if( !pSci )
        chkInFiles.SetCheck(true);

    cmbReplace.Enable(chkReplace.IsChecked());
    btnReplaceOpt.Enable(chkReplace.IsChecked());

    cmbFile.Enable(chkInFiles.IsChecked());
    cmbPath.Enable(chkInFiles.IsChecked());
    btnBrowse.Enable(chkInFiles.IsChecked());

    if( isModeRE() ) {
        chkWholeWord.SetCheck(false);
        chkWholeWord.Enable(false);
    } else
        chkWholeWord.Enable(true);

    bool b = !chkInFiles.IsChecked();
    rbtnUp.Enable(b);
    rbtnDown.Enable(b);
    if( !rbtnUp.IsEnabled() ) {
        rbtnUp.SetCheck(false);
        rbtnDown.SetCheck(true);
    }
    if( !rbtnDown.IsChecked() && !rbtnUp.IsChecked() )
        rbtnDown.SetCheck(true);

    rbtnSelection.Enable(!chkInFiles.IsChecked() && pSci && pSci->hasSel());
    rbtnWhole.Enable(!chkInFiles.IsChecked());
    if( bPrepare && rbtnSelection.IsEnabled() && pSci && pSci->hasSelLines() ) {
        rbtnSelection.SetCheck(true);
        rbtnWhole.SetCheck(false);
    }
    if( !rbtnSelection.IsEnabled() ) {
        rbtnSelection.SetCheck(false);
        rbtnWhole.SetCheck(true);
    }
    if( !rbtnWhole.IsChecked() && !rbtnSelection.IsChecked() )
        rbtnWhole.SetCheck(true);

    chkSubFolders.Enable(chkInFiles.IsChecked());
    chkCreateBak.Enable(false); // NYI: replace in files

    btnSearch.Enable(true);
    btnReplace.Enable(chkReplace.IsChecked() && !chkInFiles.IsChecked());
    btnReplaceAll.Enable(chkReplace.IsChecked() && !chkInFiles.IsChecked());

    btnSearchOpt.SetVisible(!isModeNormal());
    btnReplaceOpt.SetVisible(!isModeNormal());
}

int CFindDlg::getFindFlags() {
    int flags = 0;
    if( chkWholeWord.IsChecked() ) flags |= SCFIND_WHOLEWORD;
    if( chkMatchCase.IsChecked() ) flags |= SCFIND_MATCHCASE;
    if( isModeSciRE() ) flags |= SCFIND_REGEXP | SCFIND_POSIX;
    if( isModeStdRE() ) flags |= SCFIND_REGEXP | SCFIND_CXX11REGEX;
    return flags;
}

#define OPT_CASE  0x01
#define OPT_WORD  0x02
#define OPT_MODE  4      // shift of the mode

LPARAM CFindDlg::getOptions() {
    LPARAM opts = m_mode << OPT_MODE;
    if( chkMatchCase.IsChecked() ) opts |= OPT_CASE;
    if( chkWholeWord.IsChecked() ) opts |= OPT_WORD;
    return opts;
}

void CFindDlg::setOptions(LPARAM opts) {
    chkMatchCase.SetCheck((opts & OPT_CASE) != 0);
    chkWholeWord.SetCheck((opts & OPT_WORD) != 0);
    setMode((int)(opts >> OPT_MODE) & 0x0F);
}

static const wchar_t* modeNames[] = {L"Normal", LR"(Extended \r \n \t)", L"Scintilla regexp", L"C++11 regex"};

void CFindDlg::setMode(int mode) {
    if( mode < 0 || mode > 3 )
        mode = 0;
    m_mode = mode;
    btnMode.SetText(isModeExt() ? "\\t" : "*.");
    btnMode.SetCheck(!isModeNormal());

    wstring wsTip = tr(wstring(modeNames[mode]));
    TOOLINFOW ti = {sizeof(ti)};
    ti.hwnd = m_hWnd;
    ti.uId = (UINT_PTR) btnMode.hWnd();
    ti.lpszText = (LPWSTR) wsTip.c_str();
    ::SendMessageW(m_hTip, TTM_UPDATETIPTEXTW, 0, (LPARAM) &ti);
}

void CFindDlg::popupMode() {
    CMenu popup;
    popup.Create(true);
    for( int i = 0; i < 4; i++ ) {
        popup.Append(i + 1, tr(wstring(modeNames[i])));
        popup.Check(i + 1, i == m_mode);
    }
    CRect r = btnMode.GetRect();
    int ret = popup.TrackPopup(TPM_RETURNCMD, r.left, r.bottom, m_hWnd);
    setMode(ret ? ret - 1 : m_mode); // Escape: the same mode
}

string CFindDlg::getFindStr() {
    CSciWrapper* pSci = pEditor->GetScintilla();
    string s;
    if( !pSci ) return s;
    if( pSci->isEncodingUtf() )
        s = w2utf(cmbSearch.GetTextW());
    else
        s = cmbSearch.GetText();
    if( !s.empty() ) {
        cmbSearch.UpdateMRU(10, getOptions());
        if( isModeExt() )
            extended2string(s);
    }
    return s;
}

string CFindDlg::getFindStr(const wstring& ws, bool bAnsi) {
    string s = bAnsi ? w2a(ws) : w2utf(ws);
    if( isModeExt() )
        extended2string(s);
    return s;
}

string CFindDlg::getReplaceStr() {
    CSciWrapper* pSci = pEditor->GetScintilla();
    string s;
    if( !pSci ) return s;
    if( pSci->isEncodingUtf() )
        s = w2utf(cmbReplace.GetTextW());
    else
        s = cmbReplace.GetText();
    cmbReplace.UpdateMRU();
    if( isModeExt() )
        extended2string(s);
    return s;
}

CFindParams CFindDlg::getParams() {
    CFindParams params;
    params.wsFind = cmbSearch.GetTextW();
    params.wsReplace = cmbReplace.GetTextW();
    params.wsFiles = cmbFile.GetTextW();
    params.wsPath = cmbPath.GetTextW();
    params.opts = getOptions();
    params.bReplace = chkReplace.IsChecked();
    params.bInFiles = chkInFiles.IsChecked();
    params.bSubFolders = chkSubFolders.IsChecked();
    params.bSelection = rbtnSelection.IsChecked();
    return params;
}

bool CFindDlg::resultsInNewTab() {
    int i = pEditor->Results().IndexOf(m_pRerun);
    m_pRerun = nullptr;
    if( i < 0 )
        return chkNewTab.IsChecked();
    pEditor->Results().Select(i);
    return false;
}

bool CFindDlg::onFind() {
    CSciWrapper* pSci = pEditor->GetScintilla();
    if( !pSci ) return false;
    int flags = getFindFlags();
    string s = getFindStr();
    if( s.empty() ) return false;

    int currentPos = pSci->GetCurrentPos();
    if( pSci->hasTarget() && currentPos == pSci->GetTargetEnd() )
        currentPos = rbtnDown.IsChecked() ? pSci->GetTargetEnd() : pSci->GetTargetStart();

    Sci_TextToFind ttf = {0};
    ttf.lpstrText = (char*) s.c_str();
    if( rbtnWhole.IsChecked() ) {
        if( rbtnDown.IsChecked() ) {
            ttf.chrg.cpMin = currentPos;
            ttf.chrg.cpMax = pSci->GetTextLength();
        } else {
            ttf.chrg.cpMin = currentPos;
            ttf.chrg.cpMax = 0;
        }
    } else {
        if( rbtnDown.IsChecked() ) {
            ttf.chrg.cpMin = pSci->GetSelectionStart();
            ttf.chrg.cpMax = pSci->GetSelectionEnd();
        } else {
            ttf.chrg.cpMin = pSci->GetSelectionEnd();
            ttf.chrg.cpMax = pSci->GetSelectionStart();
        }
    }

    CWaitCursor cur;
    bool bLoop = false;
    pSci->clearStatus();
    int ret = pSci->FindText(flags, ttf);
    if( ret == -1 && rbtnWhole.IsChecked() ) {
        if( rbtnDown.IsChecked() ) {
            ttf.chrg.cpMin = 0;
            ttf.chrg.cpMax = currentPos;
        } else {
            ttf.chrg.cpMin = pSci->GetTextLength();
            ttf.chrg.cpMax = currentPos;
        }
        ret = pSci->FindText(flags, ttf);
        bLoop = true;
    }
    if( ret != -1 ) {
        pSci->SetTarget(ttf.chrgText);
        pSci->SetSel(ttf.chrgText.cpMin, ttf.chrgText.cpMax);
        pSci->EnsureVisible(ttf.chrgText.cpMin, ttf.chrgText.cpMax);
        if( bLoop ) {
            pEditor->UpdateStatusText(tr(L"Passed the end of the file"));
            MessageBeep(MB_OK);
        } else
            pEditor->UpdateStatusText("");
    } else {
        if( pSci->badRegExp() )
            pEditor->UpdateStatusText(tr(L"Invalid regular expression"));
        else
            pEditor->UpdateStatusText(tr(L"Cannot find the string") << " '" << ttf.lpstrText << "'");
        MessageBeep(MB_ICONASTERISK);
    }
    bFind = ret != -1;
    return bFind;
}

void CFindDlg::onReplace() {
    CSciWrapper* pSci = pEditor->GetScintilla();
    if( !pSci ) return;
    if( !bFind )
        if( !onFind() )
            return;

    string sReplace = getReplaceStr();

    pSci->TargetFromSelection();
    pSci->ReplaceTarget(sReplace, isModeRE());
    pSci->SelectionFromTarget();
    pSci->SetCurrentPos(pSci->GetTargetEnd());
    bFind = false;
}

// all the matches in [from, to); bBadRE: an invalid SCFIND_CXX11REGEX
static vector<Sci_CharacterRange> findAll(CSciWrapper* pSci, int flags, const string& s, int from, int to, bool& bBadRE) {
    vector<Sci_CharacterRange> ranges;
    pSci->clearStatus();
    Sci_TextToFind ttf = {0};
    ttf.lpstrText = (char*) s.c_str();
    ttf.chrg.cpMin = from;
    ttf.chrg.cpMax = to;
    while( ttf.chrg.cpMin < ttf.chrg.cpMax && pSci->FindText(flags, ttf) >= 0 ) {
        if( ttf.chrgText.cpMax > ttf.chrgText.cpMin ) {
            ranges.push_back(ttf.chrgText);
            ttf.chrg.cpMin = ttf.chrgText.cpMax;
        } else // an empty match (^, $): go on from the next char
            ttf.chrg.cpMin = pSci->Call(SCI_POSITIONAFTER, ttf.chrgText.cpMax);
    }
    bBadRE = pSci->badRegExp();
    return ranges;
}

void CFindDlg::onFindAll() {
    CSciWrapper* pSci = pEditor->GetScintilla();
    if( !pSci ) return;
    pSci->clearIndicator(INDIC_REPLACE);
    pSci->clearIndicator(INDIC_FIND);

    int flags = getFindFlags();
    string s = getFindStr();
    if( s.empty() ) return;

    vector<Sci_CharacterRange> ranges;
    bool bBadRE;
    if( rbtnWhole.IsChecked() )
        ranges = findAll(pSci, flags, s, 0, pSci->GetTextLength(), bBadRE);
    else
        ranges = findAll(pSci, flags, s, pSci->GetSelectionStart(), pSci->GetSelectionEnd(), bBadRE);
    if( bBadRE ) {
        pEditor->UpdateStatusText(tr(L"Invalid regular expression"));
        MessageBeep(MB_ICONASTERISK);
        return;
    }
    for( const auto& r : ranges )
        pSci->IndicatorFillRange(r);
    int cnt = (int)ranges.size();
    pEditor->UpdateStatusText(Format(tr(L"%i Results were found").c_str(), cnt));
    if( !cnt )
        MessageBeep(MB_ICONASTERISK);
    bFind = false;
    pSci->ClearTarget();
    pEditor->ShowFindResults('"' + s + '"', tr("%i matches"), ranges, s, resultsInNewTab());
    if( CFindResults* pResults = pEditor->Results().Cur() )
        pResults->m_params = getParams();
    closeToResults();
}

void CFindDlg::closeToResults() {
    ShowWindow(SW_HIDE);
    ::SetFocus(pEditor->Results().hWndCur());
}

// the count of the replacements; -1 for an invalid SCFIND_CXX11REGEX
int replace(CSciWrapper* pSci, int flags, const string& sFind, const string& sReplace, bool bWholeFile,
        vector<Sci_CharacterRange>* pRanges = nullptr) {
    if( !pSci ) return 0;
    pSci->clearIndicator(INDIC_FIND);
    pSci->clearIndicator(INDIC_REPLACE);
    if( sFind.empty() ) return 0;
    bool bRegExp = flags & SCFIND_REGEXP;

    int selStart = pSci->GetSelectionStart();
    int selEnd = pSci->GetSelectionEnd();

    Sci_TextToFind ttf = {0};
    ttf.lpstrText = (char*) sFind.c_str();
    if( bWholeFile ) {
        ttf.chrg.cpMin = 0;
        ttf.chrg.cpMax = pSci->GetTextLength();
    } else {
        ttf.chrg.cpMin = pSci->GetSelectionStart();
        ttf.chrg.cpMax = pSci->GetSelectionEnd();
    }

    pSci->BeginUndoAction();
    pSci->clearStatus();
    int cnt = 0;
    while( pSci->FindText(flags, ttf) != -1 ) {
        cnt++;
        pSci->SetTarget(ttf.chrgText);
        int l1 = pSci->GetTargetLength();
        pSci->ReplaceTarget(sReplace, bRegExp);
        int l2 = pSci->GetTargetLength();
        pSci->IndicatorFillRange(pSci->GetTargetStart(), pSci->GetTargetLength());
        if( pRanges ) // the next replacements are after it, so the positions stay valid
            pRanges->push_back({pSci->GetTargetStart(), pSci->GetTargetEnd()});
        ttf.chrg.cpMin = pSci->GetTargetEnd();
        if( l1 == 0 ) // an empty match (^, $, a*): go on from the next char
            ttf.chrg.cpMin = pSci->Call(SCI_POSITIONAFTER, ttf.chrg.cpMin);
        if( bWholeFile )
            ttf.chrg.cpMax = pSci->GetTextLength();
        else {
            ttf.chrg.cpMax += l2 - l1;
            selEnd += l2 - l1;
        }
        if( ttf.chrg.cpMin >= ttf.chrg.cpMax ) break;
    }
    pSci->EndUndoAction();
    if( pSci->badRegExp() )
        cnt = -1;
    if( bWholeFile )
        pSci->SetSel(0, 0);
    else
        pSci->SetSel(selStart, selEnd);
    pSci->ClearTarget();
    return cnt;
}

void replaceRE(CSciWrapper* pSci, const string& sFind, const string& sReplace) {
    replace(pSci, SCFIND_REGEXP | SCFIND_POSIX, sFind, sReplace, true);
}
void replaceNormal(CSciWrapper* pSci, const string& sFind, const string& sReplace) {
    replace(pSci, 0, sFind, sReplace, true);
}

void CFindDlg::onReplaceAll() {
    string sFind = getFindStr(), sReplace = getReplaceStr();
    vector<Sci_CharacterRange> ranges;
    int cnt = replace(pEditor->GetScintilla(), getFindFlags(), sFind, sReplace,
                      rbtnWhole.IsChecked(), &ranges);
    if( cnt < 0 )
        pEditor->UpdateStatusText(tr(L"Invalid regular expression"));
    else
        pEditor->UpdateStatusText(Format( tr(L"%i Replacements were made").c_str(), cnt));
    if( cnt <= 0 )
        MessageBeep(MB_ICONASTERISK);
    bFind = false;
    if( cnt >= 0 && !sFind.empty() ) {
        pEditor->ShowFindResults('"' + sFind + "\" -> \"" + sReplace + '"', tr("%i replacements"), ranges, sFind, resultsInNewTab());
        if( CFindResults* pResults = pEditor->Results().Cur() )
            pResults->m_params = getParams();
    }
}

/////////////////////////////////////////////////////////////////////
// find in files

// not too big and not binary (0 chars, but UTF-16 with BOM is text)
static bool isTextFile(const string& sFile) {
    if( vGetFileSize(sFile) > 50 * 1024 * 1024 )
        return false;
    CFile f;
    if( !f.Open(sFile, true) )
        return false;
    unsigned char buf[8192];
    DWORD len = f.Read(buf, sizeof(buf));
    if( len >= 2 && ((buf[0] == 0xFF && buf[1] == 0xFE) || (buf[0] == 0xFE && buf[1] == 0xFF)) )
        return true;
    return memchr(buf, 0, len) == nullptr;
}

// the files of sDir matching the masks; the folders .git, .idea, ... are skipped
static void collectFiles(const string& sDir, const vector<string>& masks, bool bSubDirs, vector<string>& files) {
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(utf2w(sDir + "\\*").c_str(), &fd);
    if( h == INVALID_HANDLE_VALUE )
        return;
    vector<string> dirs;
    do {
        string sName = w2utf(fd.cFileName);
        if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
            if( bSubDirs && sName[0] != '.' && !(fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_REPARSE_POINT)) )
                dirs.push_back(sName);
        } else
            for( const auto& sMask : masks )
                if( sMask == "*" || isFileMask(sMask, sName) ) {
                    files.push_back(sDir + '\\' + sName);
                    break;
                }
    } while( FindNextFileW(h, &fd) );
    FindClose(h);
    for( const auto& sSub : dirs ) // files first, then the subfolders
        collectFiles(sDir + '\\' + sSub, masks, bSubDirs, files);
}

void CFindDlg::onFindInFiles() {
    wstring wsFind = cmbSearch.GetTextW();
    if( wsFind.empty() )
        return;
    cmbSearch.UpdateMRU(10, getOptions());

    // folder: the folder of the current file by default
    string sDir = trimc(w2utf(cmbPath.GetTextW()));
    CTabPage* pCur = pEditor->GetCurPage();
    if( sDir.empty() && pCur && !pCur->bNew ) {
        sDir = ExtractFilePath(pCur->sFile);
        cmbPath.SetText(utf2w(sDir));
    }
    while( !sDir.empty() && (sDir.back() == '\\' || sDir.back() == '/') )
        sDir.pop_back();
    if( !IsDirExists(sDir) ) {
        pEditor->UpdateStatusText(tr(L"Folder not found") + L" '" + utf2w(sDir) + L"'");
        MessageBeep(MB_ICONASTERISK);
        return;
    }
    cmbPath.UpdateMRU();

    // masks: "*.cpp;*.h", empty is all the files
    string sMasks = trimc(w2utf(cmbFile.GetTextW()));
    if( !sMasks.empty() )
        cmbFile.UpdateMRU();
    vector<string> masks;
    string sMask;
    for( char ch : sMasks + ';' )
        if( ch == ';' || ch == ',' || ch == ' ' ) {
            if( sMask == "*.*" )
                sMask = "*"; // also the files without extension
            if( !sMask.empty() )
                masks.push_back(toLowerVal(sMask));
            sMask.clear();
        } else
            sMask += ch;
    if( masks.empty() )
        masks.push_back("*");

    CWaitCursor cur;
    vector<string> files;
    collectFiles(sDir, masks, chkSubFolders.IsChecked(), files);

    int flags = getFindFlags();
    CFindResults& results = pEditor->Results().Prepare(w2utf(wsFind), resultsInNewTab());
    results.m_params = getParams();
    results.Begin();
    CSciWrapper sciDisk; // a file, which isn't opened in the editor, is loaded here
    bool bBadRE = false;
    for( const auto& sFile : files ) {
        CTabPage* pPage = pEditor->FindPage(sFile);
        CSciWrapper* pSrc = &sciDisk;
        if( pPage ) // the text in the editor, also not saved yet
            pSrc = &pPage->sci;
        else {
            if( !isTextFile(sFile) )
                continue;
            if( !sciDisk.hWnd() )
                sciDisk.Create(pEditor->hWnd());
            CTime tm;
            int64_t size;
            if( !sciDisk.loadFromFile(sFile, tm, size) )
                continue;
        }
        string s = getFindStr(wsFind, pSrc->getEncoding() == encAnsi);
        vector<Sci_CharacterRange> ranges = findAll(pSrc, flags, s, 0, pSrc->GetTextLength(), bBadRE);
        if( bBadRE )
            break;
        results.AddFile(*pSrc, pPage ? pPage->sci.hWnd() : nullptr, sFile, ranges, true);
    }
    if( sciDisk.hWnd() )
        sciDisk.Destroy();

    // "foo" - 12 matches, 9 lines, 3 files - Z:\src [*.cpp;*.h] - 120 files searched
    string sTitle = CFindResults::EscapeTitle('"' + w2utf(wsFind) + '"') + " - ";
    if( bBadRE )
        sTitle += tr("Invalid regular expression");
    else {
        sTitle += Format(tr("%i matches").c_str(), results.cntMatches());
        if( results.cntMatches() )
            sTitle += ", " + Format(tr("%i lines").c_str(), results.cntLines())
                    + ", " + Format(tr("%i files").c_str(), results.cntFiles());
    }
    sTitle += " - " + sDir + " [" + (sMasks.empty() ? "*.*" : sMasks) + "] - "
            + Format(tr("%i files searched").c_str(), (int)files.size());
    results.End(sTitle);
    pEditor->ShowResults(true);

    if( bBadRE )
        pEditor->UpdateStatusText(tr(L"Invalid regular expression"));
    else
        pEditor->UpdateStatusText(Format(tr(L"%i Results were found").c_str(), results.cntMatches()));
    if( !results.cntMatches() )
        MessageBeep(MB_ICONASTERISK);
    if( !bBadRE ) // to correct the regular expression
        closeToResults();
}
