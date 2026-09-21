#include "stdafx.h" 
#include "platform.h"
#include "scintilla.h"
#include "sci.h"
#include "shared/win.h"
#include "progress.h"
#include "shared/platform.h"
#include "lexers.h"
#include "options.h"
#include "export2html.h"
#include "lang.h"
#include "shared/str.h"
#include <boost/regex.hpp>
#include "sciIterator.h"

void CSciWrapper::Create( HWND hParent, int x, int y, int w, int h, WORD id ) {
    m_dpiScale = GetScaleDPI();
	hSci = CreateWindowEx(WS_EX_CLIENTEDGE,
			"Scintilla","", WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
			x,y,w,h,hParent,(HMENU)(WORD)id, g_hInst, nullptr);

	sci_fn = (int (__cdecl *)(void *,unsigned int,uintptr_t,intptr_t))SendMessage( hSci,SCI_GETDIRECTFUNCTION,0,0);
	sci_ptr = (void *)SendMessage(hSci,SCI_GETDIRECTPOINTER,0,0);

	UsePopup( false );

	Call(SCI_SETVISIBLEPOLICY, VISIBLE_STRICT | VISIBLE_SLOP, 1);

	Call( SCI_SETLEXER, 0 ); // SCLEX_CONTAINER
	// Call( SCI_SETSTYLEBITS, 7 ); // deprecated

	Call( SCI_SETTABWIDTH, g_options.iTabSize );
	Call( SCI_SETUSETABS, true );
	Call( SCI_SETINDENT, 0 ); // 0 means as tab size
//	Call( SCI_SETTABINDENTS, true );
//	Call( SCI_SETBACKSPACEUNINDENTS, true );

// cur line:
	Call( SCI_SETCARETLINEBACK, RGB(0xFF,0,0) );
	Call( SCI_SETCARETLINEBACKALPHA, 10 );
	Call( SCI_SETCARETLINEVISIBLEALWAYS, true );

// selection:
//	Call(SCI_SETSELFORE, true, RGB(0,0xFF,0) );
	Call(SCI_SETSELBACK, true, RGB(0,0,0xFF) );
	Call(SCI_SETSELALPHA, 0x40);
//	Call( SCI_SETSELEOLFILLED, false ); // false by def

// call tip
	Call( SCI_CALLTIPUSESTYLE, 1 );
	Call( SCI_CALLTIPSETFOREHLT, RGB(0xFF,0,0) );

	Margin(MARGIN_NUMS)
		.Type(SC_MARGIN_NUMBER);
	Margin(MARGIN_FOLD)
		.Type(SC_MARGIN_SYMBOL)
		.Mask(SC_MASK_FOLDERS)
		.Setsitive(1);
	ViewFold(true);

	Call( SCI_SETMODEVENTMASK, SC_MOD_INSERTTEXT|SC_MOD_DELETETEXT );

// wrap long lines
	Call( SCI_SETWRAPVISUALFLAGS, SC_WRAPVISUALFLAG_END );
//	Call( SCI_SETWRAPVISUALFLAGSLOCATION, SC_WRAPVISUALFLAGLOC_START_BY_TEXT );
//	Call( SCI_SETWRAPINDENTMODE, SC_WRAPINDENT_INDENT );

// folding
	Call( SCI_SETFOLDFLAGS, SC_FOLDFLAG_LINEAFTER_CONTRACTED );
//#ifdef _DEBUG
//	Call(SCI_SETFOLDFLAGS, SC_FOLDFLAG_LINEAFTER_CONTRACTED | SC_FOLDFLAG_LEVELNUMBERS);
//	Call(SCI_SETFOLDFLAGS, SC_FOLDFLAG_LINEAFTER_CONTRACTED | SC_FOLDFLAG_LINESTATE);
//#endif
	int fore = RGB(180, 210, 255);
	int back = RGB(0, 0, 0);
	DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_CIRCLEMINUS, fore, back );
	DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_CIRCLEPLUS, fore, back );
	DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE, fore, back );
	DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER, fore, back );
	DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_CIRCLEPLUSCONNECTED, fore, back );
	DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_CIRCLEMINUSCONNECTED, fore, back );
	DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER, fore, back );
	
	Call( SCI_MARKERENABLEHIGHLIGHT, true );

	SetEOLMode( SC_EOL_CRLF );

	Call( SCI_SETENDATLASTLINE, false );

	SetViewWS( g_options.iViewWS );
	SetViewEOL( g_options.bViewEOL );
	BraceHL( g_options.bViewBraceHL );
	ViewNums( g_options.bViewNums );
	ViewIndentGuides( g_options.bViewIndentGuides );
	SetCarretLineVisible( g_options.bViewLineHL );
	setXmlMatchedTagHL( g_options.bViewXMLMatchTagHL );
	setWrapLongLines( g_options.bViewWrapLongLines );
	setTokenHL( g_options.bViewTokenHL );

	Call( SCI_SETBUFFEREDDRAW, false );
	Call( SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITE );

//	Call( SCI_BRACEHIGHLIGHTINDICATOR, true, SCE_UNIVERSAL_TAGMATCH);
//	Call( SCI_BRACEBADLIGHTINDICATOR, true, SCE_UNIVERSAL_BADMATCH);

	// SCI_SETWHITESPACECHARS
	// SCI_SETPUNCTUATIONCHARS	

	setEncoding(m_encoding);
}

void CSciWrapper::SetLexer(int l) 
{ 
	m_iLexer=l; 
	string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$";
	chars += "�������������������������������������Ũ��������������������������";
	if( m_iLexer==LEX_XHTML )
		chars += '-';
	SetWordChars( chars );
}

void CSciWrapper::setEncoding(CEncoding enc)
{
	m_encoding=enc;
	int cp = m_encoding!=encAnsi ? SC_CP_UTF8 : 0;
	int cpCur = Call(SCI_GETCODEPAGE);
	if( cp!=cpCur )
		Call(SCI_SETCODEPAGE, cp);
}

bool isAnsi(CEncoding enc) { return enc==encAnsi; }
bool isNonAnsi(CEncoding enc) { return enc==encLatin1 || enc==encLatin2 || enc==encLatin3 || enc==encAnsiCyr; }
int enc2cp(CEncoding enc) {
	switch( enc ) {
		case encLatin1: return 28591;
		case encLatin2: return 28592;
		case encLatin3: return 28593;
		case encAnsiCyr: return 1251;
		default: return 0;
	}
}

void CSciWrapper::setEncodingAndConvert(CEncoding enc) {
	if( m_encoding==enc ) return;
	// latinN to utf8NoBOM -> change encoding
	// utf8NoBOM to latinN -> change encoding
	// latinN to utf8 -> remove encoding ?
	// utf8 to latinN -> add encoding ?
	bool bA2UTF = m_encoding==encAnsi && enc!=encAnsi;
	bool bUTF2A = m_encoding!=encAnsi && enc==encAnsi;
	if( bA2UTF || bUTF2A ) {
		string s;
		GetTextRange( 0, GetLength(), s );
		if( !s.empty() ) {
			int ret;
			if( (isAnsi(m_encoding) && isNonAnsi(enc)) || (isNonAnsi(m_encoding) && isAnsi(enc)) )
				ret = IDYES;
			else
				ret = MessageBoxW(hSci,tr(L"Convert text?").c_str(),tr(L"Confirm").c_str(),MB_YESNOCANCEL);
			if( ret==IDCANCEL ) return;
			if( ret==IDYES ) {
				if( bA2UTF )
					s = a2utf(s, enc2cp(enc));
				else
				if( bUTF2A )
					s = utf2a(s, enc2cp(m_encoding));
				BeginUndoAction();
				ClearAll();
				AddText( s );
				EndUndoAction();
			}
		}
	}
	setEncoding(enc);
}

void CSciWrapper::DetectIfUtf8NoBOM()
{
	for( int i=0, len=GetTextLength(); i<len-1; i++ ) {
		unsigned char ch = GetCharAt(i);
		if( 0xC0<=ch && ch<=0xDF ) {
			unsigned char ch1 = GetCharAt(i+1);
			if( 0x80<=ch1 && ch1<=0xBF ) {
				setEncoding(encUtf8NoBOM);
				return;
			}
		}
	}
}

struct CSciTextRange : public Sci_TextRange
{
	CSciTextRange( int start, int end, char* buf ) 
	{ chrg.cpMin=start; chrg.cpMax=end; lpstrText=buf; }
};

#define BLOCK_SIZE (1024*16)

void swap_bytes( wstring& ws )
{
	for( size_t i=0; i<ws.size(); i++ ) {
		wchar_t c = ws[i];
		BYTE byL = c & 0xFF;
		BYTE byH = (c>>8) & 0xFF;
		c = (byL<<8) + byH;
		ws[i] = c;
	}
}

bool CSciWrapper::loadFromFile( const string& sFile, CTime& tm, int64_t& size, bool bReload ) {
	CFile f;
	if( !f.Open( sFile, true ) ) {
		MsgBoxError( wstring()<< tr(L"Can't load from file")<<" '"<<sFile<<"' !\n"<< GetLastErrorText() );
		return false;
	}
	CWaitCursor cur;
	vGetFileTimeSize( sFile, tm, size );

	int pos = GetCurrentPos();
	int line = GetFirstVisibleLine();
	SetReadOnly(false);
	Call(SCI_CANCEL);
	SetUndoCollection(false);

	BeginUndoAction();
	ClearAll();
	Call(SCI_ALLOCATE, size + 1000);
	setEncoding( f.DetectEncoding() );
	char data[BLOCK_SIZE+1];
	CProgress* pProgress = NULL;
	if( size>1000000 )
		pProgress = new CProgress( "Loading", 0, size);
	int p = 0;
	while( true )
	{
		int len = f.Read(data, BLOCK_SIZE );
		if( !len ) break;
		if( m_encoding==encUtf16LE || m_encoding==encUtf16BE ) {
			wstring ws( (const wchar_t*)data, len/2 );
			if( m_encoding==encUtf16BE )
				swap_bytes(ws);
			string s = w2utf(ws);
			AddText(s);
		} else
		if( m_encoding==encLatin1 ) {
			string s( data, len );			
			AddText( a2utf(s,28591) );
		} else
		if( m_encoding==encLatin2 ) {
			string s( data, len );			
			AddText( a2utf(s,28592) );
		} else
		if( m_encoding==encLatin3 ) {
			string s( data, len );			
			AddText( a2utf(s,28593) );
		} else
		if( m_encoding==encAnsiCyr ) {
			string s( data, len );
			AddText( a2utf(s,1251) );
		} else
			AddText( data, len );
		p += len;
		if( pProgress )
			pProgress->next(p);
	}
	delete pProgress;
	f.Close();
	if( getEncoding()==encAnsi )
		DetectIfUtf8NoBOM();
	EndUndoAction();

	SetUndoCollection(true);
	EmptyUndoBuffer();
	m_encLoaded = m_encoding;
	Call(SCI_SETSAVEPOINT);

	bool isRO = GetFileAttrs(sFile) & FILE_ATTRIBUTE_READONLY;
	SetReadOnly(isRO);
	GotoPos( bReload ? pos : 0 );
	SetFirstVisibleLine( bReload ? line : 0 );
	ScrollCaret();

	detectEOL();
	AdjustNumsMargin();

	return true;
}

bool CSciWrapper::saveToFile( const string& sFile, CTime& tm, int64_t& size )
{
	BeginUndoAction();
	if( g_options.bStripTrailingSpaces )
		stripTrailingSpaces();
	// ensure.final.line.end
	// ensure.consistent.line.ends
	EndUndoAction();

	CFile f;
	if( !f.Open( sFile.c_str(), false ) )
	{
		MsgBoxError( wstring()<<tr(L"Can't save to file")<<" '"<<sFile<<"' !\n"<< GetLastErrorText() );
		return false;
	}
	CWaitCursor cur;
	f.WriteEncodingBOM(m_encoding);
	char data[BLOCK_SIZE+1];
	int grabSize;
	int lengthDoc = GetLength();
	for (int i = 0; i < lengthDoc; i += grabSize) 
	{
		grabSize = lengthDoc - i;
		if (grabSize > BLOCK_SIZE)
			grabSize = BLOCK_SIZE;
		grabSize = GetPositionBefore(i + grabSize + 1) - i;
		Call(SCI_GETTEXTRANGE, 0, (intptr_t)&CSciTextRange(i, i+grabSize, data) );
		if( m_encoding==encUtf16LE || m_encoding==encUtf16BE ) {
			string s( data, grabSize );
			wstring ws = utf2w(s);
			if( m_encoding==encUtf16BE )
				swap_bytes( ws );
			f.Write( ws );
		} else
		if( m_encoding==encLatin1 ) {
			string s( data, grabSize );
			f.Write( utf2a(s,28591) );
		} else
		if( m_encoding==encLatin2 ) {
			string s( data, grabSize );
			f.Write( utf2a(s,28592) );
		} else
		if( m_encoding==encLatin3 ) {
			string s( data, grabSize );
			f.Write( utf2a(s,28593) );
		} else
		if( m_encoding==encAnsiCyr ) {
			string s( data, grabSize );
			f.Write( utf2a(s,1251) );
		} else
			f.Write( data, grabSize );
	}
	f.Close();

	m_encLoaded = m_encoding;
	Call(SCI_SETSAVEPOINT);

	vGetFileTimeSize( sFile, tm, size );
	return true;
}

void CSciWrapper::stripTrailingSpaces(bool bMsgBox) 
{
	int count = 0;
	int maxLines = GetLineCount();
	for( int line = 0; line < maxLines; line++ ) 
	{
		int lineStart = GetLineStartPosition(line);
		int lineEnd = GetLineEndPosition(line);
		int i = lineEnd - 1;
		char ch = GetCharAt(i);
		while( (i >= lineStart) && ((ch == ' ') || (ch == '\t')) ) 
		{
			i--;
			ch = GetCharAt(i);
		}
		if (i < (lineEnd - 1)) 
		{
			count += lineEnd - (i+1);
			SetTargetStart(i+1);
			SetTargetEnd(lineEnd);
			ReplaceTarget("");
		}
	}
	if( bMsgBox )
		MsgBox( itoa(count)+" trailing space chars stipped." );
}

void CSciWrapper::detectEOL()
{
	int iCRLF, iCR, iLF;
	iCRLF=iCR=iLF=0;
	for( int i=0, lines=0; i<GetLength() && lines<10000; i++ ) 
	{
		if( GetCharAt(i)=='\n' )
		{
			if( i && GetCharAt(i-1)=='\r' )
				iCRLF++;
			else
				iLF++;
			lines++;
		}
		if( GetCharAt(i)=='\r' && i<GetLength()-1 && GetCharAt(i+1)!='\n' )
		{
			iCR++;
			lines++;
		}
	}
	if( !iCRLF && !iCR && !iLF ) iCRLF++;
	if( iLF>iCR && iLF>iCRLF ) SetEOLMode( SC_EOL_LF );
	else if( iCR>iCRLF ) SetEOLMode( SC_EOL_CR ); 
	else SetEOLMode( SC_EOL_CRLF ); 
}

void CSciWrapper::GetTextRange( int p1, int p2, string& s ) 
{ 
	s.clear();
	if( p1>=p2 ) return;
	Sci_TextRange tr; 
	tr.chrg.cpMin=p1;  
	tr.chrg.cpMax=p2;  
	char* buf = new char[p2-p1+1];
	tr.lpstrText = buf;
	Call(SCI_GETTEXTRANGE,0,(intptr_t)&tr);
	s = tr.lpstrText;
	delete[] buf;
}

int CSciWrapper::FindText( int flags, Sci_TextToFind& ttf, bool boostRegExp ) 
{ 
	if( boostRegExp ) {
		boost::regex expr(ttf.lpstrText, flags&SCFIND_MATCHCASE ? 0 : boost::regex::icase );
		CSciIterator itA(this,ttf.chrg.cpMin), itB(this,ttf.chrg.cpMax);
		boost::match_results<CSciIterator> what;
		if( !boost::regex_search(itA, itB, what, expr ) )
			return -1;
		if( what.empty() || !what[0].matched )
			return -1;
		ttf.chrgText.cpMin = what[0].first.getPos();
		ttf.chrgText.cpMax = what[0].second.getPos();
		return ttf.chrgText.cpMin;
	} else
		return Call(SCI_FINDTEXT,flags,(intptr_t)&ttf); 
}

void CSciWrapper::replaceTargetBoostRE( const string& sFind, const string& sReplace ) 
{ 
	string str = regex_replace( getTargetText(), boost::regex(sFind), sReplace );
	ReplaceTarget( str, false );
}

const char* g_szFileChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-\\/.:!@#$&*+%";
const char* g_szWordChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$";

string CSciWrapper::GetFileNameFromPoint( int x, int y )
{
	CPoint pt(x,y);
	ScreenToClient( hWnd(), &pt );
	int pos = PositionFromPoint(pt.x,pt.y);

	string sTmp = GetWordChars();
	SetWordChars(g_szFileChars);
	int posA = Call(SCI_WORDSTARTPOSITION,pos,true);
	int posB = Call(SCI_WORDENDPOSITION,pos,true);
	SetWordChars(sTmp);
	return GetTextRange( posA, posB );
}

string CSciWrapper::GetWordFromPoint( int x, int y )
{
	CPoint pt(x,y);
	ScreenToClient( hWnd(), &pt );
	return GetWordFromPosition( PositionFromPoint(pt.x,pt.y) );
}

string CSciWrapper::GetWordFromPosition( int pos, bool bLeft, bool bRight, const char* szAddChars )
{
	string sTmp = GetWordChars();
	if( szAddChars )
		SetWordChars( string(g_szWordChars)+szAddChars );
	int posA = bLeft ? Call(SCI_WORDSTARTPOSITION,pos,true) : pos;
	int posB = bRight ? Call(SCI_WORDENDPOSITION,pos,true) : pos;
	if( szAddChars )
		SetWordChars(sTmp);
	return GetTextRange( posA, posB );
}

string CSciWrapper::GetIdentFromPosition( int pos, int& p )
{
	int len = GetTextLength();
	if( len==0 )
		return "";
	string s;
	int cntBr1=0, cntBr2=0;
	for( pos; pos>0; pos-- )
	{
		char ch = GetCharAt(pos-1);

		if( ch==']' ) { if(!cntBr1) /*s =']'+s;*/ cntBr1++; continue; }
		if( ch=='[' && cntBr1 ) { cntBr1--; if(!cntBr1) /*s ='['+s;*/ continue; }
//		if( ch=='[' && !cntBr1 ) break;
		if( cntBr1 ) continue;

		if( ch==')' ) { if(!cntBr2) s=')'+s; cntBr2++; continue; }
		if( ch=='(' && cntBr2 ) { cntBr2--; if(!cntBr2) s='('+s; continue; }
//		if( ch=='(' && !cntBr2 ) break;
		if( cntBr2 ) continue;

		if( strchr("\r\n\t ",ch) ) 
			if( s.empty() ) continue; else break;
		if( !strchr(g_szWordChars,ch) ) break; // is not word char
		s = ch + s;
	}
	p = pos;
	return s;
}

bool isJSSkipStyle(CSciWrapper& sci, int pos, bool );

void CSciWrapper::positionCalltipBraces(int& pos1)
{
	int pos=pos1;
	char ch = GetCharAt(pos);
	if( ch==')' || ch=='{'|| ch=='}' ) pos--;
	int cntBrace1 = 1; // ()
	int cntBrace2 = 0; // {}
	while( pos )
	{
		if( isJSSkipStyle(*this,pos,false) )
			break;
		char ch = GetCharAt(pos);
		if( ch=='(' ) cntBrace1--;
		if( ch==')' ) cntBrace1++;
		if( ch=='{' ) cntBrace2--;
		if( ch=='}' ) cntBrace2++;
		if( !cntBrace1 && !cntBrace2 )
		{
			pos1 = pos;
			return;
		}
		pos--;
	}
}

string CSciWrapper::GetSelText()
{
	int p1 = GetSelectionStart();
	int p2 = GetSelectionEnd();
	string s;
	GetTextRange( p1, p2, s );
	return s;
}

#define BRACE_STR "(){}[]"

void CSciWrapper::DoBraceHL()
{
	int nBrace1 = -1;
	int nBrace2 = -1;
	m_bBraceMatched = false;

	if( m_bBraceHL ) 
	{
		int pos = GetCurrentPos();
		char ch = 0;
		if( pos>=0 )
		{
			ch = GetCharAt(pos);
			if( ch && strchr(BRACE_STR,ch) )
				nBrace1 = pos;
			if( pos>0 && nBrace1<0 )
			{
				ch = GetCharAt(pos-1);
				if( ch && strchr(BRACE_STR,ch) )
					nBrace1 = pos-1;
			}
		}
		if( nBrace1>=0 )
			nBrace2 = Call(SCI_BRACEMATCH,nBrace1);
	}

	if( nBrace1 == -1 && nBrace2 == -1 )
	{
		if( !m_bBraceHLd ) return;
		m_bBraceHLd = false;
		Call( SCI_BRACEHIGHLIGHT, -1, -1 );
		Call( SCI_SETHIGHLIGHTGUIDE, -1 );
	} else {
		m_bBraceHLd = true;
		if( nBrace1 != -1 && nBrace2 == -1 )
		{
			Call(SCI_BRACEBADLIGHT,nBrace1);
			Call(SCI_SETHIGHLIGHTGUIDE, -1 );
		} else {
			int style = GetStyleAt(nBrace1);
			int clrBack = Style(style).Back();
			Style(STYLE_BRACELIGHT).Back(clrBack);

			Call(SCI_BRACEHIGHLIGHT,nBrace1, nBrace2);
			int column1 = GetColumn(nBrace1);
			int column2 = GetColumn(nBrace2);
			Call(SCI_SETHIGHLIGHTGUIDE, min(column1, column2) );
			m_bBraceMatched = true;
		}
	}
}

void CSciWrapper::MatchBrace()
{
	int nBrace1 = -1;
	int nBrace2 = -1;

	int pos = GetCurrentPos();
	char ch = 0;
	if( pos>0 )
	{
		ch = GetCharAt(pos);
		if( ch && strchr(BRACE_STR,ch) )
			nBrace1 = pos;
		if( nBrace1<0 )
		{
			ch = GetCharAt(pos-1);
			if( ch && strchr(BRACE_STR,ch) )
				nBrace1 = pos-1;
		}
	}
	if( nBrace1>=0 )
		nBrace2 = Call(SCI_BRACEMATCH,nBrace1);
	if( nBrace2>=0 )
		Call(SCI_GOTOPOS,nBrace2);
}

void CSciWrapper::ReStyle()
{
	StartStyling(0);
	int line = GetFirstVisibleLine();
	line += LinesOnScreen()+2;
	int posTo = GetLineEndPosition(line);
	Colorize(0,posTo);
}

int CSciWrapper::Highlight( const string& s, bool bWholeWord, int idx )
{
	if( isHighlighted(idx) ) 
		ClearHighlight(idx);
	m_styled[idx].b = true;
	m_styled[idx].str = s;
	m_styled[idx].bWholeWord = bWholeWord;
	SetIndicatorCurrent(INDIC+idx);

	Sci_TextToFind ttf={0};
	ttf.chrg.cpMin=0;
	ttf.chrg.cpMax=GetTextLength();
	ttf.lpstrText=(char*)s.c_str();
	int flags = SCFIND_MATCHCASE;
	if( bWholeWord )
		flags |= SCFIND_WHOLEWORD;
	int cnt=0;
	while( true )
	{
		int ret = FindText( flags, ttf );
		if( ret==-1 ) break;
		IndicatorFillRange( ttf.chrgText );
		ttf.chrg.cpMin = ttf.chrgText.cpMax;
		cnt++;
	}
	return cnt;
}

void CSciWrapper::ClearHighlight(int idx)
{
	if( idx==-1 )
	{
		for( int i=0; i<INDIC_CNT; i++ )
			ClearHighlight(i);
		return;
	}
	if( !isHighlighted(idx) ) return;
	m_styled[idx].b = false;
	m_styled[idx].str.clear();
	clearIndicator(INDIC+idx);
}

void CSciWrapper::ReHighlight()
{
	CWaitCursor cur;
	for( int i=0; i<INDIC_CNT; i++ )
		if( m_styled[i].b )
			Highlight( string(m_styled[i].str), m_styled[i].bWholeWord, i );
}

char CSciWrapper::GetLastCharAtLine(int line, bool bTrim) 
{
	int p1 = GetLineStartPosition(line);
	int p2 = GetLineEndPosition(line);
	string s = GetTextRange( p1, p2 );
	if( bTrim ) trim(s);
	int len = s.length();
	return len ? s[len-1] : 0;
}

void CSciWrapper::CheckIndent( SCNotification *pNS )
{
	if( !g_options.bAutoIndent ) 
		return;
	if( (pNS->ch=='\n') || (pNS->ch=='\r' && GetEOLMode()==SC_EOL_LF) ) {
		int line = GetCurrentLine();
		if( line==0 ) 
			return;
		bool bOpen = GetLastCharAtLine(line-1)=='{';
		int indent = GetLineIndentation(line-1);
		if( bOpen ) 
			indent += g_options.iTabSize;
		SetLineIndentation(line,indent);

		int pos = GetLineIndentPosition(line);
		GotoPos(pos); 
	}
	if( pNS->ch=='}' )
	{
		int line = GetCurrentLine();
		if( line==0 ) 
			return;
		int p1 = GetLineStartPosition(line);
		int p2 = GetCurrentPos();
		string s = GetTextRange( p1, p2 );
		trim(s);
		if( s=="}" )
		{
			bool bOpen = GetLastCharAtLine(line-1)=='{';
			int indent = GetLineIndentation(line-1);
			if( !bOpen )
				indent -= g_options.iTabSize;
			if( indent<0 ) indent = 0;
			SetLineIndentation(line,indent);
		}
	}
}

void CSciWrapper::FormatIndent()
{
	if( IsReadOnly() ) return;
	CWaitCursor cur;	

	bool bSel = hasSel();
	int lineFrom, lineTo;
	if( bSel ) {
		lineFrom = LineFromPosition(GetSelectionStart());
		lineTo = LineFromPosition(GetSelectionEnd());
	} else {
		lineFrom = 0;
		lineTo = GetLineCount()-1;
	}

	int posStyled = GetEndStyled();
	int posTo = GetLineEndPosition(lineTo);
	if( posTo>posStyled )
		Colorize(posStyled, posTo);

	int tabSize = Call(SCI_GETTABWIDTH);

	CProgress progress("Format Indent", lineFrom, lineTo);
	BeginUndoAction();
	for( int line=lineFrom; line<=lineTo; line++ ) {
		progress.next(line);
		int level = FoldLevel(line);
		if( line<=lineTo && FoldLevel(line+1)<level /*&& !IsFoldFooter(line+1)*/ ) // vp: todo
			level--;
		SetLineIndentation( line, level*tabSize );
	}
	EndUndoAction();

	if( bSel )
		SetSel(PositionFromLine(lineFrom), GetLineEndPosition(lineTo));

	Call(SCI_SETXOFFSET,0);
}

void CSciWrapper::WrapLines() {
    if (IsReadOnly()) return;
	int wrapLen = 80;

	CWaitCursor cur;

	bool bSel = hasSel();
	int lineFrom, lineTo;
	if (bSel) {
		lineFrom = LineFromPosition(GetSelectionStart());
		lineTo = LineFromPosition(GetSelectionEnd());
	} else {
		lineFrom = 0;
		lineTo = GetLineCount() - 1;
	}

	string sEOL;
	switch (GetEOLMode()) {
		case SC_EOL_CR: sEOL = "\r"; break;
		case SC_EOL_LF: sEOL = "\n"; break;
		case SC_EOL_CRLF: sEOL = "\r\n"; break;
		default: sEOL = "\n";
	}

	CProgress progress("Wrap Lines", lineFrom, lineTo);
	BeginUndoAction();
	for (int line = lineFrom; line <= lineTo;) {
		progress.next(line);
		int pStart = GetLineStartPosition(line);
		int pEnd = GetLineEndPosition(line);
		int len = pEnd - pStart;
		if (len > wrapLen) {
			int pos2 = Call(SCI_WORDSTARTPOSITION, pStart + wrapLen, true);
			InsertText( pos2, sEOL.c_str() );
			++lineTo;
		}
		++line;
	}
	EndUndoAction();

	if (bSel)
		SetSel(PositionFromLine(lineFrom), GetLineEndPosition(lineTo));
}

void ensureEOL(CSciWrapper& sci, int pos)
{
	const char* szEOL = NULL;
	int EOL = 0;
	int p=pos;
	switch( sci.GetEOLMode() ) {
		case SC_EOL_CR: 
			szEOL="\r"; 
			for( ; true; ++p) {
				char ch=sci.GetCharAt(p);
				if( ch==' ' || ch=='\t' ) continue;
				if( ch=='\r' )
					++EOL;
				else
					break;
			}
			break;
		case SC_EOL_LF: 
			szEOL="\n"; 
			for( ; true; ++p) {
				char ch=sci.GetCharAt(p);
				if( ch==' ' || ch=='\t' ) continue;
				if( ch=='\n' )
					++EOL;
				else
					break;
			}
			break;
		case SC_EOL_CRLF: 
			szEOL="\r\n"; 
			for( ; true; ++p) {
				char ch=sci.GetCharAt(p);
				if( ch==' ' || ch=='\t' ) continue;
				char ch2=sci.GetCharAt(++p);
				if( ch=='\r' && ch2=='\n' )
					++EOL;
				else
					break;
			}
			break;
	}

	if( EOL==0 )
		sci.InsertText( pos, szEOL );
	else
	if( EOL>1 ) {
		sci.DeleteRange( pos, p-pos-1 );
		sci.InsertText( pos, szEOL );
	}
}

int skipChars( CSciWrapper& sci, int pos, const char* szChars )
{
	for( ; true; ++pos )
		if( !strchr(szChars,sci.GetCharAt(pos)) )
			break;
	return pos;
}

void CSciWrapper::FormatXml()
{
	if( IsReadOnly() ) return;
	CWaitCursor cur;
	CProgress progress("Format XML", 0, GetLength() );

	clearSelection();

	BeginUndoAction();
	int NORMAL = 0;
	int IN_TAG = 1;
	int IN_CLOSETAG = 2;
	int IN_TEXT = 3;
	int state = NORMAL;
	for( int pos=0; pos<GetLength(); pos++ ) {
		progress.next(pos, GetLength());
		char ch = GetCharAt(pos);
		if( (state==NORMAL || state==IN_TEXT) && ch=='<' ) {
			if( GetCharAt(pos+1)=='/' ) {
				state = IN_CLOSETAG;
				pos++;
			} else
				state = IN_TAG;
			continue;
		} else
		if( state==IN_TAG && ch=='>' ) {
			int p = skipChars(*this,pos+1," \t\r\n");
			if( GetCharAt(p)=='<' && GetCharAt(p+1)!='/' ) {
				ensureEOL(*this, pos+1);
				state = NORMAL;
			} else
				state = IN_TEXT;
			continue;
		} else
		if( state==IN_TAG && ch=='/' && GetCharAt(pos+1)=='>' ) {
			ensureEOL(*this, pos+2);
			state = NORMAL;
			pos++;
			continue;
		} else
		if( state==IN_CLOSETAG && ch=='>' ) {
			ensureEOL(*this, pos+1);
			state = NORMAL;
			continue;
		}
	}
	EndUndoAction();

	FormatIndent();
}

void CSciWrapper::EnsureVisible(int posStart, int posEnd) 
{
	int lineStart = LineFromPosition( min(posStart, posEnd) );
	int lineEnd   = LineFromPosition( max(posStart, posEnd) );
	for( int line=lineStart; line<=lineEnd; line++ )
		Call( SCI_ENSUREVISIBLE, line );
}

void CSciWrapper::doFold(bool bCollapse, bool bExpand) 
{
	int line;
	line = GetCurrentLine();
	if( line<0 ) return;
	if( !IsFoldHeader(line) ) {
		line = GetFoldParent(line);
		if( line<0 ) return;
	}
	if( bCollapse && !GetFoldExpanded(line) ) return;
	if( bExpand && GetFoldExpanded(line) ) return;
	ToggleFold( line );
}

void CSciWrapper::doFoldChild(int line, bool bToggle, bool bCollapse, bool bExpand) 
{
	if( line==-1 )
		line = GetCurrentLine();
	if( line<0 )
		return;
	bool bExpanded = !GetFoldExpanded(line);
	int last = GetLastChild(line);
	for( int i=line; i<=last; i++ )
		if( IsFoldHeader(i) ) {
			if( bCollapse && !GetFoldExpanded(i) ) continue;
			if( bExpand && GetFoldExpanded(i) ) continue;
			if( bToggle && GetFoldExpanded(i)==bExpanded ) continue;
			ToggleFold(i);
		}
}

void CSciWrapper::doFoldAll(bool bToggle, bool bCollapse, bool bExpand) 
{
	ColorizeTo( GetTextLength() );
	CWaitCursor cur;
	bool bExpanded = true;
	int lines = GetLineCount();
	for( int i=0; i<lines; i++ )
		if( IsFoldHeader(i) ) {
			bExpanded = !GetFoldExpanded(i);
			break;
		}
	for( int i=0; i<lines; i++ )
		if( IsFoldHeader(i) ) {
			if( bCollapse && !GetFoldExpanded(i) ) continue;
			if( bExpand && GetFoldExpanded(i) ) continue;
			if( bToggle && GetFoldExpanded(i)==bExpanded ) continue;
			ToggleFold(i);
		}
}

void setClipboardData( HWND hWnd, const string& str, int iFormat ) 
{
	::OpenClipboard(hWnd);
	::EmptyClipboard();
	int len = str.length();
	HGLOBAL hData = ::GlobalAlloc( GMEM_MOVEABLE|GMEM_ZEROINIT, len+1 );
	char *ptr = (char*)::GlobalLock(hData);
	memcpy( ptr, str.c_str(), len );
	ptr[len]=0;
	::GlobalUnlock(hData);
	::SetClipboardData( iFormat, hData );
	::CloseClipboard();
}

void CSciWrapper::CopyAsHTML()
{
	static UINT cfHTML = 0; 
	static bool bInit = false;
	if( !bInit ) {
		bInit = true;
		cfHTML = RegisterClipboardFormat("HTML Format");
	}
	string s = export2HTMLSelected( *this );
	LPCSTR szVer			= "Version:1.0\n";
	LPCSTR szStartHTML		= "StartHTML:%09i\n";
	LPCSTR szEndHTML		= "EndHTML:%09i\n";
	LPCSTR szStartFragment	= "StartFragment:%09i\n";
	LPCSTR szEndFragment	= "EndFragment:%09i\n";
	LPCSTR szStartSelection = "StartSelection:%09i\n";
	LPCSTR szEndSelection	= "EndSelection:%09i\n";
	int headerLen = 0;
	headerLen += strlen(szVer);
	headerLen += strlen(szStartHTML)-4+9;
	headerLen += strlen(szEndHTML)-4+9;
	headerLen += strlen(szStartFragment)-4+9;
	headerLen += strlen(szEndFragment)-4+9;
	headerLen += strlen(szStartSelection)-4+9;
	headerLen += strlen(szEndSelection)-4+9;
	LPCSTR szStartFrag = "<!--StartFragment-->\n";
	int pos = s.find(szStartFrag);
	ASSERT( pos!=string::npos );
	pos += strlen(szStartFrag);
	int pos2 = s.find("<!--EndFragment-->\n");
	ASSERT( pos2!=string::npos );
	string sOut = szVer;
	char buf[128];
	sprintf( buf, szStartHTML, headerLen ); sOut += buf;
	sprintf( buf, szEndHTML, headerLen+s.length() ); sOut += buf;
	sprintf( buf, szStartFragment, headerLen+pos ); sOut += buf;
	sprintf( buf, szEndFragment, headerLen+pos2 ); sOut += buf;
	sprintf( buf, szStartSelection, headerLen+pos ); sOut += buf;
	sprintf( buf, szEndSelection, headerLen+pos2 ); sOut += buf;
	sOut += s;
	setClipboardData( hWnd(), sOut, cfHTML );
}

void CSciWrapper::CopyAsHex()
{
	string s;
	int start = this->GetSelectionStart();
	int end = this->GetSelectionEnd();
	for( int i=start; i<end; i++ ) {
		if( i>start) s <<',';
		s << "0x" << Format("%02X",GetCharAt(i));
	}
	setClipboardData( hWnd(), s, CF_TEXT );
}

bool CSciWrapper::isFoldMatched()
{
	int line = GetCurrentLine();
	if( IsFoldHeader(line) ) return true;
/*	int lev = GetFoldLevel(line);
	int lev1 = GetFoldLevel(line+1);
	if( lev!=lev1 )
	{
		int line2 = GetFoldParent(line);
		int line3 = GetLastChild(line2);
		return line==line3;
	}*/
	return false;
}

void CSciWrapper::MatchFold()
{
	int line = GetCurrentLine();
	if( IsFoldHeader(line) )
	{
		line = GetLastChild(line);
		GotoLine(line);
	} /*else {
		int line2 = GetFoldParent(line);
		int line3 = GetLastChild(line2);
		if( line==line3 )
			GotoLine(line2);
	}*/
}

void CSciWrapper::DoTokenHL()
{
	if( m_bTokenHLSkip ) {
		m_bTokenHLSkip = false;
		m_sToken.clear();
		clearIndicator(INDIC_TOKEN);
		return;
	}
	if( !m_bTokenHL )
		return;
	int pos = GetCurrentPos();
	int posA = Call(SCI_WORDSTARTPOSITION,pos,true);
	int posB = Call(SCI_WORDENDPOSITION,pos,true);
	string s = GetTextRange( posA, posB );
	if( hasSel() )
		s.clear();
	if( m_sToken==s )
		return;
	if( m_sToken!=s && !m_sToken.empty() )
		clearIndicator(INDIC_TOKEN);
	m_sToken = s;
	if( s.empty() )
		return;

	Sci_TextToFind ttf = {0};
	ttf.chrg.cpMin = 0;
	ttf.chrg.cpMax = GetTextLength();
	ttf.lpstrText = (char*)m_sToken.c_str();
	int flags = SCFIND_MATCHCASE |SCFIND_WHOLEWORD;

	SetIndicatorCurrent(INDIC_TOKEN);
	while( FindText( flags, ttf )!=-1 )	{
		IndicatorFillRange( ttf.chrgText );
		ttf.chrg.cpMin = ttf.chrgText.cpMax;
	}
}
