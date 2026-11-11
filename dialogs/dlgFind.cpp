#include "stdafx.h"
#include "resource.h"
#include "dlgFind.h"
#include "lang.h"
#include "apeEditor.h"
#include "ShellContextMenu.h"
#include "fileMask.h"
#include "shared/platform.h"
#include <regex>

CREOpt findOptsExt[] = {
    {"Cr",  "\\r"},
    {"Lf",  "\\n"},
    {"Tab", "\\t"},
    {nullptr}
};

CREOpt* replaceOptsExt = findOptsExt;

CREOpt findOptsSciRE[] = {
    {"Any Charachter",          "."},
    {"Charachter in Range",     "[ ]"},
    {"Charachter not in Range", "[^ ]"},
    {"Start of Line",           "^"},
    {"End of Line",             "$"},
    {"Start of Word",           "\\<"},
    {"End of Word",             "\\>"},
    {"0 or more Matches",       "*"},
    {"1 or more Matches",       "+"},
    {"Tagged Region",           "( )"},
    {nullptr}
};

CREOpt replaceOptsSciRE[] = {
    {"Whole Matching Text", "\\0"},
    {"Tagged Expression 1", "\\1"},
    {"Tagged Expression 2", "\\2"},
    {"Tagged Expression 3", "\\3"},
    {"Tagged Expression 4", "\\4"},
    {"Tagged Expression 5", "\\5"},
    {"Tagged Expression 6", "\\6"},
    {"Tagged Expression 7", "\\7"},
    {"Tagged Expression 8", "\\8"},
    {"Tagged Expression 9", "\\9"},
    {nullptr}
};

CREOpt findOptsStdRE[] = {
    {"Any Charachter",          "."},
    {"Charachter in Range",     "[ ]"},
    {"Charachter not in Range", "[^ ]"},
    {"Start of Line",           "^"},
    {"End of Line",             "$"},
    {"Word Boundary",           "\\b"},
    {"0 or 1 Matches",          "?"},
    {"0 or more Matches",       "*"},
    {"1 or more Matches",       "+"},
    {"Tagged Region",           "( )"},
    {nullptr}
};

CREOpt replaceOptsStdRE[] = {
    {"Whole Matching Text", "$&"},
    {"Tagged Expression 1", "$1"},
    {"Tagged Expression 2", "$2"},
    {"Tagged Expression 3", "$3"},
    {"Tagged Expression 4", "$4"},
    {"Tagged Expression 5", "$5"},
    {"Tagged Expression 6", "$6"},
    {"Tagged Expression 7", "$7"},
    {"Tagged Expression 8", "$8"},
    {"Tagged Expression 9", "$9"},
    {nullptr}
};

void InsertText(HWND hEdit, const char* sz) {
    while( *sz )
        SendMessage(hEdit, WM_CHAR, *sz++, 0);
}

void CFindDlg::popupOptRE(CREOpt* opts, HWND hDlg, HWND hBtn, HWND hEdit) {
    CMenu popup;
    popup.Create(true);
    for( int i = 0; opts[i].szTitle && opts[i].szText; i++ ) {
        string sText = opts[i].szText;
        replaceStr(sText,"&","&&");
        popup.Append(i + 1, string(opts[i].szTitle) + '\t' + sText);
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

    cmbMode.Attach(m_hWnd, IDC_FIND_MODE);
    cmbMode.AddString(tr("Normal"));
    cmbMode.AddString(tr(R"(Extended \r \n \t)"));
    cmbMode.AddString(tr("Scintilla regex"));
    cmbMode.AddString(tr("std::regex"));
    cmbMode.SetCurSel(0);

    chkSubFolders.Attach(m_hWnd, IDC_FIND_FILE_SUBDIRS);
    chkNewPane.Attach(m_hWnd, IDC_FIND_FILE_OUT2);
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

    LoadWindowXY(m_hWnd, FIND_DLG);
    cmbSearch.LoadCombo(FIND_CMB);
    cmbReplace.LoadCombo(REPLACE_CMB);
    cmbFile.LoadCombo(FILE_CMB);
    cmbPath.LoadCombo(PATH_CMB);
}

void CFindDlg::onDestroy() {
    SaveWindowXY(m_hWnd, FIND_DLG);
    cmbSearch.SaveCombo(FIND_CMB);
    cmbReplace.SaveCombo(REPLACE_CMB);
    cmbFile.SaveCombo(FILE_CMB);
    cmbPath.SaveCombo(PATH_CMB);
}

void CFindDlg::onCommand(int cmd, int code) {
    switch( cmd ) {
        case IDCANCEL:
            ShowWindow(SW_HIDE);
            break;
        case IDC_FIND_OPTION_FIND:
            if( code == 0 ) {
                CREOpt* opts = nullptr;
                if( cmbMode.GetCurSel() == 1 ) opts = findOptsExt;
                if( cmbMode.GetCurSel() == 2 ) opts = findOptsSciRE;
                if( cmbMode.GetCurSel() == 3 ) opts = findOptsStdRE;
                if( opts )
                    popupOptRE(opts, m_hWnd, btnSearchOpt.hWnd(), cmbSearch.hWnd());
                UpdateControls();
            }
            break;
        case IDC_FIND_FILE_REPLACE:
            if( code == 0 ) {
                CREOpt* opts = nullptr;
                if( cmbMode.GetCurSel() == 1 ) opts = replaceOptsExt;
                if( cmbMode.GetCurSel() == 2 ) opts = replaceOptsSciRE;
                if( cmbMode.GetCurSel() == 3 ) opts = replaceOptsStdRE;
                if( opts )
                    popupOptRE(opts, m_hWnd, btnReplaceOpt.hWnd(), cmbReplace.hWnd());
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

    bool b = !chkInFiles.IsChecked() && !isModeStdRE();
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
    chkNewPane.Enable(false);   // NYI
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
    return flags;
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
        cmbSearch.UpdateMRU();
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
    int ret = pSci->FindText(flags, ttf, isModeStdRE());
    if( ret == -1 && rbtnWhole.IsChecked() ) {
        if( rbtnDown.IsChecked() ) {
            ttf.chrg.cpMin = 0;
            ttf.chrg.cpMax = currentPos;
        } else {
            ttf.chrg.cpMin = pSci->GetTextLength();
            ttf.chrg.cpMax = currentPos;
        }
        ret = pSci->FindText(flags, ttf, isModeStdRE());
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

    string sFind = getFindStr();
    string sReplace = getReplaceStr();

    pSci->TargetFromSelection();
    if( isModeStdRE() )
        pSci->replaceTargetRegExp(sFind, sReplace);
    else
        pSci->ReplaceTarget(sReplace, isModeSciRE());
    pSci->SelectionFromTarget();
    pSci->SetCurrentPos(pSci->GetTargetEnd());
    bFind = false;
}

// all the matches in [from, to); std::regex_error for a bad std::regex
static vector<Sci_CharacterRange> findAll(CSciWrapper* pSci, int flags, const string& s, bool bStdRE, int from, int to) {
    vector<Sci_CharacterRange> ranges;
    Sci_TextToFind ttf = {0};
    ttf.lpstrText = (char*) s.c_str();
    ttf.chrg.cpMin = from;
    ttf.chrg.cpMax = to;
    while( ttf.chrg.cpMin < ttf.chrg.cpMax && pSci->FindText(flags, ttf, bStdRE) >= 0 ) { // -2: bad Scintilla RegExp
        if( ttf.chrgText.cpMax > ttf.chrgText.cpMin ) {
            ranges.push_back(ttf.chrgText);
            ttf.chrg.cpMin = ttf.chrgText.cpMax;
        } else // an empty match (^, $): go on from the next char
            ttf.chrg.cpMin = pSci->Call(SCI_POSITIONAFTER, ttf.chrgText.cpMax);
    }
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
    try {
        if( rbtnWhole.IsChecked() )
            ranges = findAll(pSci, flags, s, isModeStdRE(), 0, pSci->GetTextLength());
        else
            ranges = findAll(pSci, flags, s, isModeStdRE(), pSci->GetSelectionStart(), pSci->GetSelectionEnd());
    } catch( std::regex_error& ) {
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
    pEditor->ShowFindResults('"' + s + '"', tr("%i matches"), ranges);
    closeToResults();
}

void CFindDlg::closeToResults() {
    ShowWindow(SW_HIDE);
    ::SetFocus(pEditor->Results().hWnd());
}

int replace(CSciWrapper* pSci, int flags, const string& sFind, const string& sReplace, bool bWholeFile, bool isModeSciRE,
        bool isModeStdRE, vector<Sci_CharacterRange>* pRanges = nullptr) {
    if( !pSci ) return 0;
    pSci->clearIndicator(INDIC_FIND);
    pSci->clearIndicator(INDIC_REPLACE);
    if( sFind.empty() ) return 0;
    if( isModeSciRE ) flags |= SCFIND_REGEXP | SCFIND_POSIX;

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
    int cnt = 0;
    while( pSci->FindText(flags, ttf, isModeStdRE) != -1 ) {
        cnt++;
        pSci->SetTarget(ttf.chrgText);
        int l1 = pSci->GetTargetLength();
        if( isModeStdRE )
            pSci->replaceTargetRegExp(sFind, sReplace);
        else
            pSci->ReplaceTarget(sReplace, isModeSciRE);
        int l2 = pSci->GetTargetLength();
        pSci->IndicatorFillRange(pSci->GetTargetStart(), pSci->GetTargetLength());
        if( pRanges ) // the next replacements are after it, so the positions stay valid
            pRanges->push_back({pSci->GetTargetStart(), pSci->GetTargetEnd()});
        ttf.chrg.cpMin = pSci->GetTargetEnd();
        if( bWholeFile )
            ttf.chrg.cpMax = pSci->GetTextLength();
        else {
            ttf.chrg.cpMax += l2 - l1;
            selEnd += l2 - l1;
        }
        if( ttf.chrg.cpMin == ttf.chrg.cpMax ) break;
    }
    pSci->EndUndoAction();
    if( bWholeFile )
        pSci->SetSel(0, 0);
    else
        pSci->SetSel(selStart, selEnd);
    pSci->ClearTarget();
    return cnt;
}

void replaceSciRegExp(CSciWrapper* pSci, const string& sFind, const string& sReplace) {
    replace(pSci, 0, sFind, sReplace, true, true, false);
}
void replaceNormal(CSciWrapper* pSci, const string& sFind, const string& sReplace) {
    replace(pSci, 0, sFind, sReplace, true, false, false);
}

void CFindDlg::onReplaceAll() {
    string sFind = getFindStr(), sReplace = getReplaceStr();
    vector<Sci_CharacterRange> ranges;
    int cnt = replace(pEditor->GetScintilla(), getFindFlags(), sFind, sReplace,
                      rbtnWhole.IsChecked(), isModeSciRE(), isModeStdRE(), &ranges);
    pEditor->UpdateStatusText(Format( tr(L"%i Replacements were made").c_str(), cnt));
    if( !cnt )
        MessageBeep(MB_ICONASTERISK);
    bFind = false;
    if( !sFind.empty() )
        pEditor->ShowFindResults('"' + sFind + "\" -> \"" + sReplace + '"', tr("%i replacements"), ranges);
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
    cmbSearch.UpdateMRU();

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
    bool bStdRE = isModeStdRE();
    CFindResults& results = pEditor->Results();
    results.Begin();
    CSciWrapper sciDisk; // a file, which isn't opened in the editor, is loaded here
    bool bBadRE = false;
    try {
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
            vector<Sci_CharacterRange> ranges = findAll(pSrc, flags, s, bStdRE, 0, pSrc->GetTextLength());
            results.AddFile(*pSrc, pPage ? pPage->sci.hWnd() : nullptr, sFile, ranges, true);
        }
    } catch( std::regex_error& ) {
        bBadRE = true;
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
