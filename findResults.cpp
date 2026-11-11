#include "stdafx.h"
#include "findResults.h"
#include "lexerStyles.h"
#include "theme.h"
#include "lang.h"
#include "shared/platform.h"

#define MAX_LINE_LEN 512 // longer source lines are cut

// Enter and Escape are for the parent
static LRESULT CALLBACK ResultsProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
	switch( msg ) {
	case WM_KEYDOWN:
		if( wParam == VK_RETURN || wParam == VK_ESCAPE ) {
			::PostMessage(::GetParent(hWnd), WM_RESULTS_KEY, wParam, 0);
			return 0;
		}
		if( wParam == VK_LEFT || wParam == VK_RIGHT ) { // on a file header: collapse/expand, else the caret moves
			auto line = ::SendMessage(hWnd, SCI_LINEFROMPOSITION, ::SendMessage(hWnd, SCI_GETCURRENTPOS, 0, 0), 0);
			bool bHeader = (::SendMessage(hWnd, SCI_GETFOLDLEVEL, line, 0) & SC_FOLDLEVELHEADERFLAG) != 0;
			bool bExpanded = ::SendMessage(hWnd, SCI_GETFOLDEXPANDED, line, 0) != 0;
			if( bHeader && bExpanded == (wParam == VK_LEFT) ) {
				::SendMessage(hWnd, SCI_TOGGLEFOLD, line, 0);
				return 0;
			}
		}
		break;
	case WM_CHAR:
		if( wParam == '\r' || wParam == VK_ESCAPE )
			return 0;
		break;
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

void CFindResults::Create(HWND hParent) {
	m_sci.Create(hParent);
	SetWindowSubclass(m_sci.hWnd(), ResultsProc, 1, 0);
	m_sci.ViewFold(false);
	m_sci.ViewNums(false);
	m_sci.SetViewWS(SCWS_INVISIBLE);
	m_sci.SetViewEOL(false);
	m_sci.BraceHL(false);
	m_sci.ViewIndentGuides(false);
	m_sci.setXmlMatchedTagHL(false);
	m_sci.setTokenHL(false);
	m_sci.setWrapLongLines(false);
	m_sci.Call(SCI_SETMARGINLEFT, 0, g_dpi.scale(4));
	m_sci.Call(SCI_SETCARETSTYLE, CARETSTYLE_INVISIBLE);
	m_sci.Call(SCI_SETUNDOCOLLECTION, false);
	m_sci.Call(SCI_SETSCROLLWIDTH, 1); // the horizontal scrollbar only for long lines
	m_sci.Call(SCI_SETSCROLLWIDTHTRACKING, true);
	m_sci.SetCarretLineVisible(true);
	m_sci.Call(SCI_SETCARETLINEVISIBLEALWAYS, true);
	m_sci.Call(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CLICK | SC_AUTOMATICFOLD_CHANGE);
	m_sci.SetReadOnly(true);
	SetupStyles();
}

void CFindResults::SetupStyles() {
	SetupDefaultStyle(m_sci);
	if( !m_sFont.empty() ) {
		m_sci.StyleDefault().Font(m_sFont);
		m_sci.Call(SCI_STYLESETSIZEFRACTIONAL, STYLE_DEFAULT, m_fontSize);
		m_sci.StylesClear();
	}
	m_sci.Style(stLineNo).Fore(g_theme.editorFore(RGB(0x80, 0x80, 0x80)));
	m_sci.Style(stMatch).Bold(true).Back(g_theme.editorBack(RGB(0xFF, 0xF0, 0xA0)));
	m_sci.Style(stTitle).Bold(true).Fore(g_theme.editorFore(RGB(0, 0, 0x80)));
	m_sci.Style(stFile).Fore(g_theme.editorFore(RGB(0, 0x60, 0)));
	COLORREF clrSymbol = g_theme.editorFore(RGB(0, 0x80, 0x80)); // as the operators in the editor
	m_sci.Style(stSymbol).Fore(clrSymbol);
	m_sci.Style(stMatchSymbol).Bold(true).Fore(clrSymbol).Back(g_theme.editorBack(RGB(0xFF, 0xF0, 0xA0)));
	// fold margin, as in the editor
	m_sci.SetupFoldMarkers();
	m_sci.Call(SCI_SETFOLDMARGINCOLOUR, g_theme.isDark(), CTheme::clrMargin);
	m_sci.Call(SCI_SETFOLDMARGINHICOLOUR, g_theme.isDark(), CTheme::clrMargin);
	// the current result line
	m_sci.Call(SCI_SETCARETLINEBACK, g_theme.isDark() ? RGB(0x60, 0x80, 0xC0) : RGB(0, 0x60, 0xFF));
	m_sci.Call(SCI_SETCARETLINEBACKALPHA, 0x30);
	m_sci.Call( SCI_SETCARETLINEVISIBLEALWAYS, true );

}

void CFindResults::UseFontOf(CSciWrapper& src) {
	char szFont[LF_FACESIZE * 3] = {};
	if( src.Call(SCI_STYLEGETFONT, STYLE_DEFAULT) < (int)sizeof(szFont) )
		src.Call(SCI_STYLEGETFONT, STYLE_DEFAULT, (intptr_t)szFont);
	int fontSize = src.Call(SCI_STYLEGETSIZEFRACTIONAL, STYLE_DEFAULT);
	if( szFont != m_sFont || fontSize != m_fontSize ) {
		m_sFont = szFont;
		m_fontSize = fontSize;
		SetupStyles();
	}
}

string CFindResults::EscapeTitle(string s) {
	replaceStr(s, "\r", "\\r"); // extended mode: one line
	replaceStr(s, "\n", "\\n");
	replaceStr(s, "\t", "\\t");
	return s;
}

void CFindResults::add(const string& s, char style) {
	for( char ch : s ) {
		m_cells += ch;
		m_cells += style;
	}
}

void CFindResults::addText(const string& s, bool bMatch) {
	for( char ch : s ) {
		bool bSymbol = (unsigned char)ch < 0x80 && ispunct((unsigned char)ch); // utf-8 bytes are not symbols
		m_cells += ch;
		m_cells += bSymbol ? (bMatch ? stMatchSymbol : stSymbol) : (bMatch ? stMatch : stDefault);
	}
}

void CFindResults::addLine() {
	add("\r\n", stDefault);
}

void CFindResults::Clear() {
	m_results.clear();
	m_lineToResult.clear();
	ResetShown();
	m_sci.SetReadOnly(false);
	m_sci.ClearAll();
	m_sci.SetReadOnly(true);
}

void CFindResults::onSciClosed(HWND hSci) {
	for( auto& res : m_results )
		if( res.hSci == hSci )
			res.hSci = nullptr;
}

void CFindResults::Fill(const string& sWhat, const string& sMatches, CSciWrapper& src, const string& sFile,
		const vector<Sci_CharacterRange>& ranges) {
	UseFontOf(src);
	Begin();
	AddFile(src, src.hWnd(), sFile, ranges, false);

	string sTitle = EscapeTitle(src.getEncoding() == encAnsi ? a2utf(sWhat) : sWhat);
	sTitle += " - " + Format(sMatches.c_str(), m_cntMatches);
	if( !m_results.empty() )
		sTitle += ", " + Format(tr("%i lines").c_str(), (int)m_results.size());
	if( !sFile.empty() )
		sTitle += " - " + ExtractFileName(sFile);
	End(sTitle);
}

void CFindResults::Begin() {
	m_bFilling = true;
	m_results.clear();
	m_lineToResult.clear();
	m_lineToResult.push_back(-1); // the title
	m_cells.clear();
	m_cntMatches = m_cntFiles = 0;
	m_bHeaders = false;
	ResetShown();
}

void CFindResults::AddFile(CSciWrapper& src, HWND hSci, const string& sFile, const vector<Sci_CharacterRange>& ranges,
		bool bHeader) {
	if( ranges.empty() )
		return;
	m_cntFiles++;
	m_cntMatches += (int)ranges.size();

	// the pane is utf-8; ANSI is the only encoding, which Scintilla doesn't keep as utf-8
	bool bAnsi = src.getEncoding() == encAnsi;
	auto conv = [bAnsi](const string& s) { return bAnsi ? a2utf(s) : s; };

	// group the matches by lines
	size_t first = m_results.size();
	for( const auto& r : ranges ) {
		int line = src.LineFromPosition(r.cpMin);
		if( m_results.size() == first || m_results.back().line != line )
			m_results.push_back({sFile, hSci, line});
		int lineStart = src.PositionFromLine(line);
		m_results.back().matches.push_back({r.cpMin - lineStart, r.cpMax - r.cpMin, -1});
	}

	if( bHeader ) {
		m_bHeaders = true;
		addLine();
		m_lineToResult.push_back(-1);
		add(sFile, stFile);
		add(" (" + to_string(ranges.size()) + ")", stLineNo);
	}

	int width = (int)to_string(m_results.back().line + 1).size();
	for( size_t i = first; i < m_results.size(); i++ ) {
		CFindResult& res = m_results[i];
		addLine();
		m_lineToResult.push_back((int)i);
		size_t lineCells = m_cells.size(); // the result line starts here

		string sLine = src.GetTextRange(src.PositionFromLine(res.line), src.GetLineEndPosition(res.line));
		int trim = (int)min(sLine.find_first_not_of(" \t"), sLine.size()); // leading spaces aren't shown
		sLine.erase(0, trim);
		if( sLine.size() > MAX_LINE_LEN ) {
			size_t len = MAX_LINE_LEN;
			while( !bAnsi && len > 0 && ((unsigned char)sLine[len] & 0xC0) == 0x80 ) // don't cut a utf-8 char
				len--;
			sLine.resize(len);
		}

		string sNo = to_string(res.line + 1);
		add(string(2 + width - sNo.size(), ' ') + sNo + "  ", stLineNo);

		// plain and bold parts are converted separately, so the bold ones stay where they are
		int p = 0, len = (int)sLine.size();
		for( auto& m : res.matches ) {
			int p1 = max(m.offset - trim, p);
			int p2 = min(m.offset - trim + m.len, len);
			if( p1 >= len )
				break;
			addText(conv(sLine.substr(p, p1 - p)), false);
			m.col = (int)(m_cells.size() - lineCells) / 2;
			if( p2 > p1 )
				addText(conv(sLine.substr(p1, p2 - p1)), true);
			p = max(p, p2);
		}
		addText(conv(sLine.substr(p)), false);
	}
}

void CFindResults::End(const string& sTitle) {
	string sBody;
	sBody.swap(m_cells);
	add(sTitle, stTitle); // the first line
	m_cells += sBody;

	m_sci.SetReadOnly(false);
	m_sci.ClearAll();
	m_sci.Call(SCI_SETSCROLLWIDTH, 1);
	m_sci.Call(SCI_ADDSTYLEDTEXT, m_cells.size(), (intptr_t)m_cells.data());
	m_sci.SetReadOnly(true);
	m_cells.clear();
	m_cells.shrink_to_fit();

	// folding: a file header folds its result lines, the title isn't a header
	for( int line = 0; line < (int)m_lineToResult.size(); line++ ) {
		int level = SC_FOLDLEVELBASE;
		if( line > 0 && m_lineToResult[line] < 0 )
			level = SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG; // top level for SCI_FOLDALL
		else
		if( line > 0 )
			level = SC_FOLDLEVELBASE + 1;
		m_sci.SetFoldLevel(line, level);
	}
	m_sci.ViewFold(m_bHeaders);

	m_sci.GotoPos(0);
	m_bFilling = false;
}

const CFindResult* CFindResults::GetCurResult(int& iMatch) {
	iMatch = -1;
	int pos = m_sci.GetCurrentPos();
	int line = m_sci.LineFromPosition(pos);
	if( line < 0 || line >= (int)m_lineToResult.size() || m_lineToResult[line] < 0 )
		return nullptr;
	const CFindResult& res = m_results[m_lineToResult[line]];
	// the last match, which starts before the caret (clicked in a match or after it)
	int col = pos - m_sci.PositionFromLine(line);
	iMatch = 0;
	for( size_t i = 1; i < res.matches.size(); i++ )
		if( res.matches[i].col >= 0 && res.matches[i].col <= col )
			iMatch = (int)i;
	return &res;
}

bool CFindResults::SelectNext(bool bNext) {
	int line = m_sci.GetCurrentLine();
	for( line += bNext ? 1 : -1; 0 <= line && line < (int)m_lineToResult.size(); line += bNext ? 1 : -1 )
		if( m_lineToResult[line] >= 0 ) {
			m_sci.Call(SCI_ENSUREVISIBLE, line); // the file can be collapsed
			m_sci.GotoLine(line);
			return true;
		}
	return false;
}

bool CFindResults::isNewResult() {
	int iMatch;
	const CFindResult* pRes = GetCurResult(iMatch);
	int iRes = pRes ? (int)(pRes - m_results.data()) : -1;
	if( iRes == m_iShown && iMatch == m_iShownMatch )
		return false;
	m_iShown = iRes;
	m_iShownMatch = iMatch;
	return true;
}

bool CFindResults::ToggleFoldAtCaret() {
	int line = m_sci.GetCurrentLine();
	if( !m_sci.IsFoldHeader(line) )
		return false;
	m_sci.ToggleFold(line);
	return true;
}

void CFindResults::FoldAll(bool bCollapse) {
	m_sci.Call(SCI_FOLDALL, bCollapse ? SC_FOLDACTION_CONTRACT : SC_FOLDACTION_EXPAND);
}
