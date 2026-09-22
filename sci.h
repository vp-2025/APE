#pragma once

#include <windows.h>
#include <cmath>

#include "scintilla.h"

#include "shared/ctime.h"
#include "shared/cfile.h"
#include "xmlMatchedTagHL.h"
using namespace std;

#define MARGIN_NUMS 1
#define MARGIN_FOLD 2

#define INDIC 1
#define INDIC_CNT 7
#define INDIC_FIND 11
#define INDIC_REPLACE 12
#define INDIC_TOKEN 13
#define INDIC_TAGMATCH 14
#define INDIC_TAGATTR 15


class CScintilla {
public:
	virtual int Call( unsigned int msg, uintptr_t wParam=0, intptr_t lParam=0 ) const = 0;
};

class CSciStyle {
	int idx;
	CScintilla& sci;
public:
	CSciStyle(CScintilla& sci_, int idx_) : sci(sci_), idx(idx_) {}

	CSciStyle& Font(LPCSTR font) { sci.Call(SCI_STYLESETFONT,idx,(intptr_t)font); return *this; }
	CSciStyle& Font( const string& s ) { Font(s.c_str()); return *this; }
	CSciStyle& Size( int size ) { sci.Call(SCI_STYLESETSIZE,idx,size); return *this; }
	CSciStyle& Bold( bool b ) { sci.Call(SCI_STYLESETBOLD,idx,b); return *this; }
	CSciStyle& Italic( bool b ) { sci.Call(SCI_STYLESETITALIC,idx,b); return *this; }
	CSciStyle& Underline( bool b ) { sci.Call(SCI_STYLESETUNDERLINE,idx,b); return *this; }
	CSciStyle& EOL( bool b ) { sci.Call(SCI_STYLESETEOLFILLED,idx,b); return *this; }
	CSciStyle& Fore( int clr ) { sci.Call(SCI_STYLESETFORE,idx,clr); return *this; }
	CSciStyle& Back( int clr ) { sci.Call(SCI_STYLESETBACK,idx,clr); return *this; }
	CSciStyle& HotSpot( bool b ) { sci.Call(SCI_STYLESETHOTSPOT,idx,b); return *this; }

	int Back() { return sci.Call(SCI_STYLEGETBACK,idx); }
};

class CSciMargin {
	int idx;
	CScintilla& sci;
public:
	CSciMargin(CScintilla& sci_, int idx_) : sci(sci_), idx(idx_) {}

	CSciMargin& Type(int type) { sci.Call(SCI_SETMARGINTYPEN,idx,type); return *this; }
	CSciMargin& Width(int w) { sci.Call(SCI_SETMARGINWIDTHN,idx,w); return *this; }
	CSciMargin& Mask(int mask) { sci.Call(SCI_SETMARGINMASKN,idx,mask); return *this; }
	CSciMargin& Setsitive(bool b) { sci.Call(SCI_SETMARGINSENSITIVEN,idx,b); return *this; }
	CSciMargin& Cursor(int cur) { sci.Call(SCI_SETMARGINCURSORN,idx,cur); return *this; }

	int Width() { return sci.Call(SCI_GETMARGINWIDTHN,idx); }
};

class CSciIndicator {
	int idx;
	CScintilla& sci;
public:
	CSciIndicator(CScintilla& sci_, int idx_) : sci(sci_), idx(idx_) {}

	CSciIndicator& Style(int style) { sci.Call(SCI_INDICSETSTYLE, idx, style); return *this; }
	CSciIndicator& Fore(int clr) { sci.Call(SCI_INDICSETFORE, idx, clr); return *this; }
	CSciIndicator& Alpha(int alpha) { sci.Call(SCI_INDICSETALPHA, idx, alpha); return *this; }
	CSciIndicator& Alpha(double alpha) { return Alpha( (int)alpha ); }
	CSciIndicator& OutlineAlpha(int alpha) { sci.Call(SCI_INDICSETOUTLINEALPHA, idx, alpha); return *this; }
	CSciIndicator& OutlineAlpha(double alpha) { return OutlineAlpha( (int)alpha ); }
	CSciIndicator& Under(bool b) { sci.Call(SCI_INDICSETUNDER, idx, b); return *this; }
};

class CSciWrapper : public CScintilla
{
	HWND hSci;
	int (*sci_fn)(void*,unsigned int,uintptr_t,intptr_t);
	void* sci_ptr;
	int m_iLexer;
	bool m_bBraceHL, m_bBraceHLd, m_bBraceMatched; // Brace Highlight flags
	struct CStyled {
		bool b;
		string str;
		bool bWholeWord;
		CStyled() { b=bWholeWord=false; }
	} m_styled[INDIC_CNT];
	CEncoding m_encoding, m_encLoaded;
    float m_dpiScale;
public:
	CEncoding getEncoding() { return m_encoding; }
	bool isEncodingUtf() { return m_encoding==encUtf8 || m_encoding==encUtf8NoBOM; }
	void setEncoding(CEncoding enc);
	void setEncodingAndConvert(CEncoding enc);
	void DetectIfUtf8NoBOM();

	string getEncodingString() { 
		switch(m_encoding){
			case encAnsi: return "ANSI"; 
			case encUtf8: return "UTF-8"; 
			case encUtf8NoBOM: return "UTF-8 w/o BOM";
			case encUtf16BE: return "UTF-16 Big Endian"; 
			case encUtf16LE: return "UTF-16 Little Endian";
			case encLatin1: return "Latin-1 (ISO 8859-1)"; 
			case encLatin2: return "Latin-2 (ISO 8859-2)"; 
			case encLatin3: return "Latin-3 (ISO 8859-3)";
			case encAnsiCyr: return "ANSI Cyrillic (1251)";
			default: return "";
		}
	}
	string getEOLString() { 
		switch(GetEOLMode()) {
			case SC_EOL_CRLF: return "Windows (\\r\\n)";
			case SC_EOL_LF: return "Unix (\\n)";
			case SC_EOL_CR: return "Max (\\r)";
			default: return "";
		}
	}

	bool isModified() const { return m_encoding!=m_encLoaded || GetModify(); }

	CSciWrapper() {
		hSci=nullptr; sci_fn=nullptr; sci_ptr=nullptr;
		m_iLexer=0; 
		m_bBraceHL=m_bBraceHLd=m_bBraceMatched=false; 
		m_encoding=m_encLoaded=encUtf8NoBOM; //encAnsi; 
		m_bXmlMatchedTagHL=m_bTokenHL=m_bTokenHLSkip=false;
	}

	void Create( HWND hParent, int x=0, int y=0, int w=0, int h=0, WORD id=0 );
	void Destroy() { 
		DestroyWindow(hSci);
	}

	HWND hWnd() { return hSci; }

	bool isBraceMatched() const { return m_bBraceMatched; }
	void MatchBrace();

	bool isFoldMatched();
	void MatchFold();

	string GetSelText();
	bool hasSel() { return GetSelectionStart()!=GetSelectionEnd(); }
	bool hasSelLines() { return LineFromPosition(GetSelectionStart())!=LineFromPosition(GetSelectionEnd()); }
	bool hasSelection(int cnt=0) 
	{ 
		int p1=GetSelectionStart(), p2=GetSelectionEnd();
		if( p1==p2 ) return false;
		if( LineFromPosition(p1)!=LineFromPosition(p2) ) return false;
		return cnt ? (p2-p1<cnt) : true;
	}
	void clearSelection() { 
		int p1 = GetSelectionStart();
		int p2 = GetSelectionEnd();
		if( p1!=p2 ) SetSel(p1,p1); 
	}

	void GetCurrentPosLineCol( int& pos, int& line, int& col ) 
	{ 
		pos=GetCurrentPos(); 
		line=LineFromPosition(pos); 
		col=GetColumn(pos); 
	}

	bool loadFromFile( const string& sFile, CTime& tm, int64_t& size, bool bReload=false );
	bool saveToFile( const string& sFile, CTime& tm, int64_t& size );
	void stripTrailingSpaces(bool bMsgBox=false);

	string GetFileNameFromPoint( int x, int y );
	string GetWordFromPoint( int x, int y );
	string GetWordFromPosition( int pos, bool bLeft=true, bool bRight=true, const char* szAddChars=NULL );
	string GetIdentFromPosition( int pos, int& p );
	void positionCalltipBraces( int& pos );

	void ViewFold(bool b) { Margin(MARGIN_FOLD).Width(b?16:0); }
	bool isViewFold() { return Margin(MARGIN_FOLD).Width()>0; }

	void ViewNums(bool b) { Margin(MARGIN_NUMS).Width(b?1:0); AdjustNumsMargin(); }
	bool isViewNums() { return Margin(MARGIN_NUMS).Width()>0; }
	void AdjustNumsMargin() {
		if( !isViewNums() ) return;
		int n=(int)log10((double)GetLineCount())+1; 
		Margin(MARGIN_NUMS).Width(
//#ifdef _DEBUG
//			max(n*10,70)
//#else
            (int)(n*10*m_dpiScale)
//#endif
		);
	}

	void ReStyle();

	bool isHighlighted(int idx) { return m_styled[idx].b; }
	string getHighlighted(int idx) { return m_styled[idx].str; }
	int Highlight( const string& s, bool bWholeWord, int idx );
	void ClearHighlight(int idx);
	void ReHighlight();

	char GetLastCharAtLine(int line, bool trim=true);
	void CheckIndent( SCNotification *pNS );

	void FormatIndent();
	void FormatXml();
	void WrapLines();

	void EnsureVisible(int posStart, int posEnd);

	void CopyAsHTML();
	void CopyAsHex();

// functions from Capital are calls to Scintilla; Scintilla functions are splited in groups (like in Doc)
	int Call( unsigned int msg, uintptr_t wParam=0, intptr_t lParam=0 ) const
	{ return sci_fn( sci_ptr, msg, wParam, lParam ); }

	void AddText( const char* sz, int len=-1 ) {
		if( len==-1 ) len = strlen(sz);
		Call( SCI_ADDTEXT, len, (intptr_t)sz );
	}
	void AddText( const string& s ) { 
		AddText( s.c_str(), s.size() );
	}
	void InsertText( int pos, const char* sz ) { Call( SCI_INSERTTEXT, pos, (intptr_t)sz ); }
	void DeleteRange( int pos, int len ) { Call(SCI_DELETERANGE, pos, len); }

// Text retrieval and modification
	void ClearAll() { Call(SCI_CLEARALL); }
	unsigned char GetCharAt(int pos) { return Call(SCI_GETCHARAT,pos); }
	int GetStyleAt(int pos) { return Call(SCI_GETSTYLEAT,pos); }
	bool IsReadOnly() { return Call(SCI_GETREADONLY)!=0; }
	void SetReadOnly(bool b) { Call(SCI_SETREADONLY,b); }
	void SwitchReadOnly() { SetReadOnly(!IsReadOnly()); }

	void GetTextRange( int p1, int p2, string& s );
	string GetTextRange( int p1, int p2 ) { string s; GetTextRange(p1,p2,s); return s; }

	void ReplaceSel( const string& s ) { Call(SCI_REPLACESEL, 0, (intptr_t)s.c_str() ); }

// Searching
	int FindText( int flags, Sci_TextToFind& ttf, bool regExp=false );

// Search and replace using the target
	bool hasTarget() { return GetTargetStart()!=GetTargetEnd(); }
	void ClearTarget() { SetTargetStart(0); SetTargetEnd(0); }
	void SetTarget(const Sci_CharacterRange& chrg) { Call(SCI_SETTARGETRANGE, chrg.cpMin, chrg.cpMax); }
	void SetTarget(int t1, int t2) { SetTargetStart(t1); SetTargetEnd(t2); }
	void SetTargetStart(int pos) { Call(SCI_SETTARGETSTART,pos); }
	int GetTargetStart() { return Call(SCI_GETTARGETSTART); }
	void SetTargetEnd(int pos) { Call(SCI_SETTARGETEND,pos); }
	int GetTargetEnd() { return Call(SCI_GETTARGETEND); }
	int GetTargetLength() { return GetTargetEnd()-GetTargetStart(); }
	void TargetFromSelection() { Call(SCI_TARGETFROMSELECTION); }
	void SelectionFromTarget() { SetSel(GetTargetStart(),GetTargetEnd()); }
	void ReplaceTarget(const string& s, bool bRegExp=false) { Call( bRegExp?SCI_REPLACETARGETRE:SCI_REPLACETARGET, -1, (intptr_t)s.c_str() ); }

	string getTargetText() { return GetTextRange(GetTargetStart(),GetTargetEnd()); }
	void replaceTargetRegExp( const string& sFind, const string& sReplace );

// Overtype
	void SetOverType(bool b) { Call(SCI_SETOVERTYPE,b); }
	bool GetOverType() { return Call(SCI_GETOVERTYPE)!=0; }

// Cut, copy and paste
	void Cut() { Call(SCI_CUT); }
	void Copy() { Call(SCI_COPY); }
	void Paste() { Call(SCI_PASTE); }
	bool CanPaste() { return Call(SCI_CANPASTE)!=0; }
	void Delete() { Call(SCI_CLEAR); }
	void SelectAll() { Call(SCI_SELECTALL); }

// Undo and Redo
	void Undo() { Call(SCI_UNDO); }
	bool CanUndo() { return Call(SCI_CANUNDO)!=0; }
	void Redo() { Call(SCI_REDO); }
	bool CanRedo() { return Call(SCI_CANREDO)!=0; }
	void EmptyUndoBuffer() { Call(SCI_EMPTYUNDOBUFFER); }
	void SetUndoCollection(bool collectUndo) { Call(SCI_SETUNDOCOLLECTION,collectUndo); }
	bool GetUndoCollection() { return Call(SCI_GETUNDOCOLLECTION)!=0; }
	void BeginUndoAction() { Call(SCI_BEGINUNDOACTION); }
	void EndUndoAction() { Call(SCI_ENDUNDOACTION); }

// Selection and information
	bool GetModify() const { return Call(SCI_GETMODIFY)!=0; }
	int GetLength() { return Call(SCI_GETLENGTH); }
	int GetTextLength() { return Call(SCI_GETTEXTLENGTH); }
	int GetSelectionStart() { return Call(SCI_GETSELECTIONSTART); }
	int GetSelectionEnd() { return Call(SCI_GETSELECTIONEND); }
	void SetSel(int p,int p2) { Call(SCI_SETSEL,p,p2); }
	bool IsSelectionRectangle() { return Call(SCI_SELECTIONISRECTANGLE)!=0; }
	int LinesOnScreen() { return Call(SCI_LINESONSCREEN); }
	int GetFirstVisibleLine() { return Call(SCI_GETFIRSTVISIBLELINE); }
	void SetFirstVisibleLine(int line) { Call(SCI_SETFIRSTVISIBLELINE,line); }
	int GetLineCount() { return Call(SCI_GETLINECOUNT); }
	int GetColumn(int pos) { return Call(SCI_GETCOLUMN,pos); }
	int GetCurrentPos() { return Call(SCI_GETCURRENTPOS); }
	void SetCurrentPos(int pos) { Call(SCI_SETCURRENTPOS,pos); }
	int LineFromPosition(int pos) { return Call(SCI_LINEFROMPOSITION,pos); }
	int PositionFromLine(int line) { return Call(SCI_POSITIONFROMLINE,line); }
	int GetLineStartPosition(int line) { return PositionFromLine(line); }
	int GetLineEndPosition(int line) { return Call(SCI_GETLINEENDPOSITION,line); }
	int GetCurrentLine() { return LineFromPosition(GetCurrentPos()); }
	int GetAnchor() { return Call(SCI_GETANCHOR); }
	void SetAnchor(int anch) { Call(SCI_SETANCHOR,anch); }
	int PositionFromPoint(int x,int y) { return Call(SCI_POSITIONFROMPOINT,x,y); }
	void GotoPos(int pos) { Call(SCI_GOTOPOS,pos); }
	void GotoLine(int line) { Call(SCI_GOTOLINE,line); }
	int GetPositionBefore(int pos) { return Call(SCI_POSITIONBEFORE,pos); }
	int GetPositionAfter(int pos) { return Call(SCI_POSITIONAFTER,pos); }

// Scrolling and automatic scrolling
	void LineScroll(int column,int line) { Call(SCI_LINESCROLL,column,line); }
	void ScrollCaret() { Call(SCI_SCROLLCARET); }

// White space
	void SetViewWS(int wsMode) { Call(SCI_SETVIEWWS,wsMode); }
	int GetViewWS() { return Call(SCI_GETVIEWWS); }
//
	void viewWSNone() { SetViewWS(SCWS_INVISIBLE); }
	void viewWSPart() { SetViewWS(SCWS_VISIBLEAFTERINDENT); }
	void viewWSAll() { SetViewWS(SCWS_VISIBLEALWAYS); }
	bool isViewWSNone() { return GetViewWS()==SCWS_INVISIBLE; }
	bool isViewWSPart() { return GetViewWS()==SCWS_VISIBLEAFTERINDENT; }
	bool isViewWSAll() { return GetViewWS()==SCWS_VISIBLEALWAYS; }

// Line endings
	void SetEOLMode(int eolMode) { Call(SCI_SETEOLMODE,eolMode); }
	int GetEOLMode() { return Call(SCI_GETEOLMODE); }
	const char* GetEOLStr() { 
		switch( Call(SCI_GETEOLMODE) ) { 
			case SC_EOL_CRLF: return "\r\n"; 
			case SC_EOL_CR: return "\r"; 
			case SC_EOL_LF: return "\n"; 
		}
		return "";
	}
	void ConvertEOLs(int eolMode) { Call(SCI_CONVERTEOLS,eolMode); }
	void SetViewEOL(bool b) { Call(SCI_SETVIEWEOL,b); }
	bool GetViewEOL() { return Call(SCI_GETVIEWEOL)!=0; }

	void setEOLMode(int eolMode) { SetEOLMode(eolMode); ConvertEOLs(eolMode); }
	void detectEOL();

// Style
	void StyleResetDefault() { Call(SCI_STYLERESETDEFAULT); }
	CSciStyle StyleDefault() { return CSciStyle(*this,STYLE_DEFAULT); }
	void StylesClear() { Call(SCI_STYLECLEARALL); }
	CSciStyle Style(int idx) { return CSciStyle(*this,idx); }

// Margin
	CSciMargin Margin(int idx) { return CSciMargin(*this,idx); }

// Indicators
	CSciIndicator Indicator(int idx) { return CSciIndicator(*this,idx); }

// Caret, selection, and hotspot styles
	void SetCarretLineVisible(bool b) { Call(SCI_SETCARETLINEVISIBLE,b); }
	bool GetCarretLineVisible() { return Call(SCI_GETCARETLINEVISIBLE)!=0; }

// folding
	int GetFoldLevel(int line) { return Call(SCI_GETFOLDLEVEL,line); }
	void SetFoldLevel(int line,int level) { Call(SCI_SETFOLDLEVEL,line,level); }
	bool GetFoldExpanded(int line) { return Call(SCI_GETFOLDEXPANDED,line)!=0; }
	void SetFoldExpanded(int line,bool b) { Call(SCI_SETFOLDEXPANDED,line,b); }
	void ToggleFold(int line) { Call(SCI_TOGGLEFOLD,line); }
	int GetLastChild(int line) { return Call(SCI_GETLASTCHILD,line,-1); }
	bool IsFoldHeader(int line) { return (GetFoldLevel(line)&SC_FOLDLEVELHEADERFLAG)!=0; }
	int GetFoldParent(int line) { return Call(SCI_GETFOLDPARENT,line); }
	int FoldLevel(int line) { return GetFoldLevel(line)&SC_FOLDLEVELNUMBERMASK-SC_FOLDLEVELBASE; } // 	// used for Statusbar
	void doFold(bool bCollapse, bool bExpand);
	void doFoldChild(int line, bool bToggle, bool bCollapse, bool bExpand);
	void doFoldAll(bool bToggle, bool bCollapse, bool bExpand);

// Brace
	bool isBraceHL() { return m_bBraceHL; }
	void BraceHL(bool b) { m_bBraceHL=b; DoBraceHL(); }
	void DoBraceHL();

// Tabs and Indentation Guides
	void SetLineIndentation(int line,int indent) { Call(SCI_SETLINEINDENTATION,line,indent); }
	int GetLineIndentation(int line) { return Call(SCI_GETLINEINDENTATION,line); }
	int GetLineIndentPosition(int line) { return Call(SCI_GETLINEINDENTPOSITION,line); }

	void ViewIndentGuides(bool b) { Call(SCI_SETINDENTATIONGUIDES,b?SC_IV_LOOKBOTH:SC_IV_NONE); }
	bool isViewIndentGuides() { return Call(SCI_GETINDENTATIONGUIDES)!=SC_IV_NONE; }

// Markers
	void DefineMarker( int marker, int markerType, int fore, int back )
	{
		Call(SCI_MARKERDEFINE, marker, markerType);
		Call(SCI_MARKERSETFORE, marker, fore);
		Call(SCI_MARKERSETBACK, marker, back);
	}

// Popup edit menu
	void UsePopup( bool b ) { Call(SCI_USEPOPUP,b); }

// lexer & styling
	void Colorize(int from,int to) { Call(SCI_COLOURISE,from,to); }
	void ColorizeTo(int to) { Call(SCI_COLOURISE,GetEndStyled(),to); }
	int GetEndStyled() { return Call(SCI_GETENDSTYLED); }
	void StartStyling(int position, int mask=0x7F) { Call(SCI_STARTSTYLING,position,mask); }
	void SetStyling(int length, int style) { Call(SCI_SETSTYLING,length,style); }
	void SetLineState(int line, int value) { Call(SCI_SETLINESTATE,line,value); }
	int GetLineState(int line) { return Call(SCI_GETLINESTATE,line); }

	int Lexer() { return m_iLexer; }
	void SetLexer(int l);

// autoComplete
	void AutoCShow(int len, const string& s) { Call(SCI_AUTOCSHOW,len,(intptr_t)s.c_str()); }
	void AutoCSelect(const string& s) { Call(SCI_AUTOCSELECT,0,(intptr_t)s.c_str()); }
	void AutoComplete(const string& sItems, const string& sSelect) { if(sItems.empty()) return; AutoCShow(sSelect.size(), sItems); if(!sSelect.empty()) AutoCSelect(sSelect); }

	void ClearRegisteredImages() { Call(SCI_CLEARREGISTEREDIMAGES); }
	void RegisterImage(int idx, const char** sz) { Call(SCI_REGISTERIMAGE,idx,(intptr_t)sz); }

// Calltip
	void CalltipShow(int pos, const string& s) { Call(SCI_CALLTIPSHOW, pos, (intptr_t)s.c_str() ); }
	void CalltipSetHlt(int start, int end) { Call( SCI_CALLTIPSETHLT, start, end ); }
	void CalltipCancel() { if( Call(SCI_CALLTIPACTIVE) ) Call(SCI_CALLTIPCANCEL); }

// keyboard functions
	void UpperCase() { Call(SCI_UPPERCASE); }
	void LowerCase() { Call(SCI_LOWERCASE); }

// Zoom
	void ZoomIn() { Call(SCI_ZOOMIN); }
	void ZoomOut() { Call(SCI_ZOOMOUT); }
	int GetZoom() { return Call(SCI_GETZOOM); }
	void SetZoom(int z) { Call(SCI_SETZOOM,z); }

// Indicators
	void SetIndicatorCurrent(int k) { Call(SCI_SETINDICATORCURRENT, k); }
	void IndicatorFillRange(int pos, int len) { Call(SCI_INDICATORFILLRANGE, pos, len); }
	void IndicatorFillRange(const Sci_CharacterRange& chrg) { IndicatorFillRange(chrg.cpMin, chrg.cpMax-chrg.cpMin); }
	void IndicatorClearRange( int pos, int len ) { Call(SCI_INDICATORCLEARRANGE, pos, len); }
	void setIndicator(int i, int pos, int len ) { SetIndicatorCurrent(i); IndicatorFillRange(pos,len); }
	void clearIndicator(int i) { SetIndicatorCurrent(i); IndicatorClearRange(0,GetTextLength()); }

	void SetHScrollBar( bool b ) { Call( SCI_SETHSCROLLBAR, b ); }
	void SetVScrollbar( bool b ) { Call( SCI_SETVSCROLLBAR, b ); }

	void setWrapLongLines( bool b ) { Call(SCI_SETWRAPMODE, b?SC_WRAP_WORD:SC_WRAP_NONE ); }
	bool getWrapLongLines() { return Call(SCI_GETWRAPMODE)>0; }

private:
	bool m_bXmlMatchedTagHL;
	CXmlMatchedTagsHighlighter m_xmlTag;
public:
	bool isXmlMatchedTagHL() { return m_bXmlMatchedTagHL; }
	void setXmlMatchedTagHL(bool b) { m_bXmlMatchedTagHL=b; if(b) m_xmlTag.tagHighlight(this,true); else m_xmlTag.tagClear(this); }
	void XmlMatchedTagHL() { if( m_bXmlMatchedTagHL ) m_xmlTag.tagHighlight(this,true); }

private:
	bool m_bTokenHL, m_bTokenHLSkip;
	string m_sToken;
public:
	bool isTokenHL() { return m_bTokenHL; }
	void setTokenHL(bool b) { m_bTokenHL=b; if(b) DoTokenHL(); else {m_sToken.clear(); clearIndicator(INDIC_TOKEN);} }
	void DoTokenHL();
	void DoTokenHLSkip() { m_bTokenHLSkip=true; }

	string GetWordChars() { char buf[1024]; int k=Call(SCI_GETWORDCHARS,0,(LPARAM)buf); buf[k]=0; return buf; }
	void SetWordChars(const char* sz) { Call(SCI_SETWORDCHARS,0,(LPARAM)sz); }
	void SetWordChars(const string& s) { SetWordChars(s.c_str()); }
};