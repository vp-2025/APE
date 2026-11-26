#pragma once

#include "sci.h"

// Table: the current file as a table, View > Table; a list view at the bottom of the main window, above the search results
// every line of the file is a row (row == line), the cells are separated by \t, ';' or ',' (auto-detected or chosen)
// "quoted" cells can contain the separator, "" is a quote
// the header of a column is its 1-based index; the list is virtual (LVS_OWNERDATA): the cells are split when shown
// one cell is selected (not the whole row): the row is the selected item, the column is m_selCol, drawn by NM_CUSTOMDRAW;
// Left/Right move the cell, the caret in the editor selects its cell
// Ctrl+click on a cell selects its text in the editor; Escape closes the table
// the text of a cell is drawn in the styles of the editor (colors, bold, italic), like the outline
// the list column 0 is the line number (in the style of the line numbers of the editor), the cells are the list columns 1..m_cols:
// the data column col is the list column col+1; m_selCol, m_cols, Home/End, GetCellRange() are data columns

class CTable {
public:
	enum CSepMode { sepAuto, sepTab, sepSemicolon, sepComma };
	struct CCell { int start, end; bool bQuoted; }; // bytes from the line start; a quoted one without the quotes
private:
	HWND m_hWnd{};
	HWND m_hSci{};       // the editor, the table is built for
	bool m_bAnsi{};      // the encoding of the editor: ANSI is not utf-8 in Scintilla
	char m_sep{'\t'};    // the separator in use
	int m_cols{};
	int m_rows{};
	bool m_bFilling{};
	// the styles of the editor, read when first used; cleared by Fill() and applyTheme()
	struct CAttr { bool bInit; COLORREF fore, back; bool bBack; int font; } m_attrs[256]{};
	HFONT m_hFonts[4]{}; // [bold | italic<<1]; [0] is not used: the font of the list
	const CAttr& attr(int style);
	void drawCell(HDC hDC, int row, int col, COLORREF back, COLORREF foreSel); // foreSel -1: the colors of the styles
	void drawLineNo(HDC hDC, int row, bool bCurRow);
	vector<int> dataOrder(); // the data columns by the display order of the list (the columns can be dragged)
	int lineNoWidth();
	// cache: the last split line, LVN_GETDISPINFO asks for all the cells of a row one by one
	int m_cacheLine{-1};
	string m_cacheText;
	vector<CCell> m_cacheCells;
	static constexpr int maxCols = 1000;   // more columns are not shown
	static constexpr int maxScanLines = 100000; // the column count is taken from the first lines only

	int m_selCol{};      // the column of the selected cell (data column, not the display order)

	const vector<CCell>& cells(int line, string* pText = nullptr);
	void invalidateRow(int row);
	static LRESULT CALLBACK ListProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR ref);
public:
	~CTable() { for( HFONT h : m_hFonts ) if( h ) DeleteObject(h); }
	static int SplitterH() { return g_dpi.scale(5); }
	int m_height{200};      // with the splitter
	bool m_bVisible{};      // View > Table; it is shown only if there is an editor
	int m_sepMode{sepAuto}; // the popup menu of the table

	static char DetectSep(CSciWrapper& sci);
	static void Split(const string& s, char sep, vector<CCell>& cells);

	void Create(HWND hParent, HFONT hFont);
	HWND hWnd() const { return m_hWnd; }
	HWND hSci() const { return m_hSci; }
	char sep() const { return m_sep; }
	bool hasFocus() const { return m_hWnd && ::GetFocus() == m_hWnd; }
	bool isFilling() const { return m_bFilling; }
	void Show(bool bShow);
	bool isShown() const { return (GetWindowLong(m_hWnd, GWL_STYLE) & WS_VISIBLE) != 0; }
	void Move(int x, int y, int w, int h);
	void applyTheme();

	void Fill(CSciWrapper& sci); // the columns and the row count, the cells are read when shown
	void Clear();
	void onSciClosed(HWND hSci);
	// the cell of the line and the byte offset in the line (-1: the column is kept), scrolled into view; the editor is not moved
	void SelectCell(int line, int offset = -1);
	int selRow() const { return ListView_GetNextItem(m_hWnd, -1, LVNI_FOCUSED); }
	int selCol() const { return m_selCol; }
	void SetSelCol(int col);     // + scrolled into view
	void MoveSelCol(int delta);  // Left/Right: by the display order of the columns
	LRESULT onCustomDraw(NMLVCUSTOMDRAW* pCD); // only the selected cell is highlighted
private:
	LRESULT onHeaderDraw(NMCUSTOMDRAW* pCD);   // the header: its own background, the column of the selected cell marked
	void invalidateHeader() { InvalidateRect(ListView_GetHeader(m_hWnd), nullptr, FALSE); }
public:

	// the cell of the editor text: positions in the editor; false if there is no such cell
	bool GetCellRange(CSciWrapper& sci, int row, int col, int& p1, int& p2);
	void onGetDispInfo(NMLVDISPINFOW* pDI);
};
