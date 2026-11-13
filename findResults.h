#pragma once

#include "sci.h"
#include "tabs.h"
#include <memory>

// Find/Replace results: a read-only Scintilla at the bottom of the main window
//   "foo" - 3 matches, 2 lines - main.cpp
//     12  some text with foo and foo
//    140  the other foo
// in files, every file has a header line: "Z:\src\main.cpp (2)", it folds the lines of the file
// matches are bold, a result line shows/opens the file in the editor
// CResultsTabs: every search can have its own tab, the caption is the search string

#define WM_RESULTS_KEY (WM_USER+10) // to the parent: Enter or Escape in the results, wParam is the key

struct CFindMatch {
	int offset; // from the start of the source line
	int len;
	int col;    // where it is shown in the result line, -1 if it is cut
};

// the search, which filled the results: Rerun opens the find dialog with it
struct CFindParams {
	wstring wsFind, wsReplace, wsFiles, wsPath; // as typed in the dialog
	LPARAM opts{};       // match case, whole word, mode: CFindDlg::getOptions()
	bool bReplace{};     // Replace All
	bool bInFiles{};
	bool bSubFolders{};
	bool bSelection{};   // in the selection, not in the whole file
};

struct CFindResult {
	string sFile;   // to open the file, if its tab is closed
	HWND hSci;      // source editor, to find the tab (also an untitled one); nullptr: file on disk
	int line;       // source line (0-based)
	vector<CFindMatch> matches;
};

class CFindResults {
	CSciWrapper m_sci;
	vector<CFindResult> m_results;
	vector<int> m_lineToResult; // result line -> index in m_results or -1 (title, file)
	string m_cells;             // text + styles for SCI_ADDSTYLEDTEXT
	int m_iShown{-1}, m_iShownMatch{-1};
	bool m_bFilling{};
	int m_cntMatches{}, m_cntFiles{};
	bool m_bHeaders{}; // file header lines: folding

	void add(const string& s, char style);
	void addText(const string& s, bool bMatch); // source text: the symbols get their own style
	void addLine();
public:
	enum { stDefault, stLineNo, stMatch, stTitle, stFile, stSymbol, stMatchSymbol }; // styleFindResults[]
	string m_sTitle; // the first line, utf-8: the tooltip of the tab
	CFindParams m_params;

	void Create(HWND hParent);
	void Destroy() { m_sci.Destroy(); }
	HWND hWnd() { return m_sci.hWnd(); }
	CSciWrapper& sci() { return m_sci; }
	bool isEmpty() const { return m_results.empty(); }
	void SetupStyles(); // Options > Colors > Search Results; on theme change

	// one file: sWhat is the start of the title, in the source encoding: "foo";
	// sMatches is a format with %i: "%i matches"; ranges are sorted positions in src
	void Fill(const string& sWhat, const string& sMatches, CSciWrapper& src, const string& sFile,
			const vector<Sci_CharacterRange>& ranges);

	// many files: Begin(), AddFile() for every file, End()
	void Begin();
	// hSci is the editor of the file or nullptr; bHeader: a line with the file name
	void AddFile(CSciWrapper& src, HWND hSci, const string& sFile, const vector<Sci_CharacterRange>& ranges,
			bool bHeader);
	void End(const string& sTitle); // utf-8
	int cntMatches() const { return m_cntMatches; }
	int cntLines() const { return (int)m_results.size(); }
	int cntFiles() const { return m_cntFiles; }
	static string EscapeTitle(string s); // \r \n \t are shown as such

	void Clear();
	void onSciClosed(HWND hSci); // the tab is closed, its file is opened again by the next goto

	// result under the caret, or nullptr; iMatch is the match nearest to the caret
	const CFindResult* GetCurResult(int& iMatch);
	// moves the caret to the next/prev result line; false if there is none
	bool SelectNext(bool bNext);
	// true if the caret result is not the one shown last time
	bool isNewResult();
	void ResetShown() { m_iShown = m_iShownMatch = -1; }

	// folding of the files
	bool ToggleFoldAtCaret(); // true if the caret is on a file header
	void FoldAll(bool bCollapse);
	bool hasFolding() const { return m_bHeaders; }

	bool isFilling() const { return m_bFilling; }
};

#define ID_RESULTS_TABS 223 // the id of the tab control, wParam of WM_TAB_*

// the find results at the bottom of the main window: tabs, the current tab's results under them
class CResultsTabs {
	CTabCtrlAdv m_tabs;
	vector<unique_ptr<CFindResults>> m_vTabs; // in the order of the tabs
	int m_iCur{-1};
	HWND m_hParent{};
	CRect m_rect; // tabs + results, in the parent
public:
	static int SplitterH() { return g_dpi.scale(5); }
	int m_height{200}; // with the splitter
	bool m_bVisible{};

	void Create(HWND hParent);
	CTabCtrlAdv& tabs() { return m_tabs; }
	int count() const { return (int)m_vTabs.size(); }
	CFindResults* Cur() { return m_iCur >= 0 ? m_vTabs[m_iCur].get() : nullptr; }
	CFindResults* Get(int i) { return 0 <= i && i < count() ? m_vTabs[i].get() : nullptr; }
	CFindResults* FromHwnd(HWND hSci); // the results with this Scintilla
	int IndexOf(const CFindResults* p) const; // -1 if it is closed
	HWND hWndCur() { return Cur() ? Cur()->hWnd() : nullptr; }
	bool hasFocus() { return hWndCur() && ::GetFocus() == hWndCur(); }
	bool isEmpty() { return !Cur() || Cur()->isEmpty(); }

	// the tab for the next search: a new one or the current one; sCaption is the search string, utf-8
	CFindResults& Prepare(const string& sCaption, bool bNewTab);
	void Select(int i);
	void Close(int i);
	void Reorder(); // after drag & drop of a tab
	void Show(bool bShow);
	void Move(int x, int y, int w, int h);
	void DrawItem(DRAWITEMSTRUCT* pDIS) { m_tabs.DrawItem(pDIS, false, false); }

	void SetupStyles();
	void applyTheme(); // + scrollbars, border, tooltips
	void onSciClosed(HWND hSci);
};
