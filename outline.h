#pragma once

#include "sci.h"
#include <map>
#include <set>

// Outline: a tree view at the right of the main window, View > Outline
// the tree is the folding of the current file: every line is an item, a fold header has the lines of its fold as children
// blank lines have no item; also the lines closing a fold ("}", "};", "</div>", "end"), unless m_bShowClosing
// the item text is the header line; "{" alone on the line gets the previous line before it: "void foo() {"
// the item of the caret line is selected; navigating the tree moves the caret to the line (if m_bSelectLine)
// the folding is the same: expand/collapse in the tree folds the editor, SyncFold() applies the editor folding to the tree

class COutline {
	HWND m_hWnd{};
	HWND m_hSci{};                          // the editor, the tree is built for
	std::map<int, HTREEITEM> m_lineToItem;  // line -> item
	std::map<HTREEITEM, int> m_headers;     // item of a fold header -> the header line
	bool m_bFilling{};
	static constexpr int maxAllLines = 20000; // a bigger file: only the fold headers, the tree would be too slow

public:
	static int SplitterW() { return g_dpi.scale(5); }
	int m_width{250};         // with the splitter
	bool m_bVisible{};        // View > Outline; it is shown only if there is an editor
	bool m_bSelectLine{true}; // navigating the tree selects the line in the editor, the focus stays in the tree
	bool m_bShowClosing{};    // the popup menu of the outline: Show Closing Lines

	void Create(HWND hParent, HFONT hFont);
	HWND hWnd() const { return m_hWnd; }
	HWND hSci() const { return m_hSci; }
	bool hasFocus() const { return m_hWnd && ::GetFocus() == m_hWnd; }
	bool isFilling() const { return m_bFilling; }
	void Show(bool bShow);    // the window, m_bVisible is not changed
	bool isShown() const { return (GetWindowLong(m_hWnd, GWL_STYLE) & WS_VISIBLE) != 0; }
	void Move(int x, int y, int w, int h);
	void applyTheme();

	void Fill(CSciWrapper& sci); // rebuilds the tree from the fold levels
	void Clear();
	void onSciClosed(HWND hSci);

	void SelectLine(CSciWrapper& sci, int line); // the innermost item containing the line, the editor is not moved
	int GetLine(HTREEITEM h);   // the line of the item, -1 if none
	int GetSelLine() { return GetLine(TreeView_GetSelection(m_hWnd)); }
	void SyncFold(CSciWrapper& sci); // the editor folding -> expanded/collapsed items
	void onItemExpanded(const NMTREEVIEWW* p, CSciWrapper& sci); // by the user: folds the editor; with Shift also the children
};
