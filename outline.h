#pragma once

#include "sci.h"
#include <map>
#include <set>

// Outline: a tree view at the right of the main window, View > Outline
// the tree is the folding of the current file: every line is an item, a fold header has the lines of its fold as children
// blank lines have no item; also the lines closing a fold ("}", "};", "</div>", "end"), unless m_bShowClosing
// the item text is the header line; "{" alone on the line gets the previous line before it: "void foo() {"
// the item of the caret line is selected; navigating the tree moves the caret to the line (if m_bSelectLine)
// m_bSyncFold: the folding is the same: expand/collapse in the tree folds the editor, SyncFold() applies the editor folding to the tree;
// else the tree has its own folding: the collapsed items are kept, when the tree is rebuilt
// every item is drawn as its line in the editor: the runs of the styles (colors, backgrounds, bold, italic): NM_CUSTOMDRAW

class COutline {
	HWND m_hWnd{};
	HWND m_hSci{};                          // the editor, the tree is built for
	std::map<int, HTREEITEM> m_lineToItem;  // line -> item
	std::map<HTREEITEM, int> m_headers;     // item of a fold header -> the header line
public:
	struct CRun { int len; COLORREF fore, back; bool bBack; int font; }; // fore -1: the text color of the tree
private:
	std::map<HTREEITEM, vector<CRun>> m_styled; // every item: its text by the styles of the editor
	HFONT m_hFonts[4]{};                    // [bold | italic<<1]; [0] is not used: the font of the tree
	std::map<HWND, std::set<wstring>> m_collapsed; // !m_bSyncFold: per editor, the paths of the collapsed items
	bool m_bFilling{};
	static constexpr int maxAllLines = 20000; // a bigger file: only the fold headers, the tree would be too slow

	wstring ItemText(HTREEITEM h);
	wstring ItemPath(HTREEITEM h); // the texts from the root, '\n' separated
	void saveCollapsed();          // !m_bSyncFold: before the tree is cleared
	void deleteAll();              // all the items, fast
	void expandChildren(HTREEITEM h, bool bExpand); // all the children of the item

public:
	~COutline() { for( HFONT h : m_hFonts ) if( h ) DeleteObject(h); }
	static int SplitterW() { return g_dpi.scale(5); }
	int m_width{250};         // with the splitter
	bool m_bVisible{};        // View > Outline; it is shown only if there is an editor
	bool m_bSelectLine{true}; // navigating the tree selects the line in the editor, the focus stays in the tree
	bool m_bSyncFold{true};   // the popup menu of the outline: Sync Folding with Editor
	bool m_bShowClosing{};    // the popup menu of the outline: Show Closing Lines

	void Create(HWND hParent, HFONT hFont);
	HWND hWnd() const { return m_hWnd; }
	HWND hSci() const { return m_hSci; }
	bool hasFocus() const { return m_hWnd && ::GetFocus() == m_hWnd; }
	bool isFilling() const { return m_bFilling; }
	void Show(bool bShow) const { ::ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE); }    // the window, m_bVisible is not changed
	bool isShown() const { return (GetWindowLong(m_hWnd, GWL_STYLE) & WS_VISIBLE) != 0; }
	void Move(int x, int y, int w, int h) { ::MoveWindow(m_hWnd, x, y, max(w, 0), max(h, 0), TRUE); }
	void applyTheme();

	void Fill(CSciWrapper& sci); // rebuilds the tree from the fold levels
	void Clear();
	void onSciClosed(HWND hSci);

	void SelectLine(CSciWrapper& sci, int line); // the innermost item containing the line, the editor is not moved
	int GetLine(HTREEITEM h);   // the line of the item, -1 if none
	int GetSelLine() { return GetLine(TreeView_GetSelection(m_hWnd)); }
	void SyncFold(CSciWrapper& sci); // m_bSyncFold: the editor folding -> expanded/collapsed items
	void onItemExpanded(const NMTREEVIEWW* p, CSciWrapper& sci); // by the user: folds the editor (m_bSyncFold); with Shift also the children
	void ExpandAll(bool bExpand);    // only the tree: !m_bSyncFold
	LRESULT onCustomDraw(NMTVCUSTOMDRAW* pCD); // the items as their lines in the editor
private:
	void drawStyled(NMTVCUSTOMDRAW* pCD, const vector<CRun>& runs);
	COLORREF treeBack();          // the background color of the tree
	bool isSelected(HTREEITEM h) { return h && h == TreeView_GetSelection(m_hWnd); }  // the selected item: drawn as the selected cell of the table
};
