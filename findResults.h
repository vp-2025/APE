#pragma once

#include "sci.h"

// Find/Replace results: a read-only Scintilla at the bottom of the main window
//   "foo" - 3 matches, 2 lines - main.cpp
//     12  some text with foo and foo
//    140  the other foo
// in files, every file has a header line: "Z:\src\main.cpp (2)", it folds the lines of the file
// matches are bold, a result line shows/opens the file in the editor

#define WM_RESULTS_KEY (WM_USER+10) // to the parent: Enter or Escape in the results, wParam is the key

struct CFindMatch {
	int offset; // from the start of the source line
	int len;
	int col;    // where it is shown in the result line, -1 if it is cut
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
	static int SplitterH() { return g_dpi.scale(5); }
	int m_height{200}; // with the splitter
	bool m_bVisible{};

	void Create(HWND hParent);
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
