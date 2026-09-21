#pragma once

#include "lexBase.h"
#include "sci.h"
#include <cctype>

class CMyLexerBase {
protected:
	enum EnumStyle {
		defaultStyleMask = 0x7F,
		styleInvalidText = 31,
		INITIAL = 0
	};

	CSciWrapper& m_sci;
	int m_state,
		m_pos,
		m_matchLen,
		m_line,			
		m_level,		// fold level
		m_levelInc,		// fold inc++
		m_levelDec,		// fold dec--
		m_levelIncreasedFirst;	// -1 = undef, 1 = inc, 0 = dec

	CProgress* m_pProgress;
public:
	CMyLexerBase(CSciWrapper& sci) : m_sci(sci)
	{
		m_pProgress = NULL;
		m_pos = m_sci.GetEndStyled();
		m_line = m_sci.LineFromPosition(m_pos);
		m_line--;
		if (m_sci.GetFoldLevel(m_line) & SC_FOLDLEVELHEADERFLAG)
			m_line--;
		if (m_line<0) m_line = 0;
		m_pos = m_sci.PositionFromLine(m_line); // trim to begin of line

		if (m_pos <= 0) {
			m_level = SC_FOLDLEVELBASE;
			m_state = INITIAL;
		} else {
			m_level = m_sci.GetFoldLevel(m_line) & SC_FOLDLEVELNUMBERMASK;
			m_state = m_line > 0 ? m_sci.GetLineState(m_line) : INITIAL;
		}
		m_levelInc = m_levelDec = 0;
		m_levelIncreasedFirst = -1;

		m_sci.StartStyling(m_pos);
		m_matchLen = 0;
	}

	void AfterAction()
	{
		int line1 = m_sci.LineFromPosition(m_pos);
		assert(m_matchLen>0);
		m_pos += m_matchLen;
		m_matchLen = 0;

		char ch = m_sci.GetCharAt(m_pos-1);
		if ( ch == '\n' || (ch == '\r' && m_sci.GetCharAt(m_pos) != '\n') ) {
			int line2 = m_sci.LineFromPosition(m_pos);
			// after
			m_sci.SetLineState(m_line, m_state);

			// before
			int foldFlags = 0;
			int levelDiff = m_levelInc - m_levelDec;
			if (levelDiff > 0)
				foldFlags |= SC_FOLDLEVELHEADERFLAG;
			if (m_levelInc && m_levelDec &&
				m_levelIncreasedFirst == 0 &&
				levelDiff >= 0)
			{
				foldFlags |= SC_FOLDLEVELHEADERFLAG | SC_FOLDLEVELFOOTERFLAG;
				m_level--;
				levelDiff++;
			}
			m_sci.SetFoldLevel(m_line, m_level | foldFlags );
			m_levelInc = m_levelDec = 0;
			m_levelIncreasedFirst = -1;
			m_level += levelDiff;
			m_line++;
		}
	}

	void StyleTo(int posTo)
	{
		if (posTo - m_pos > 1000000)
			m_pProgress = new CProgress("Styling", m_pos, posTo);

		while (m_pos<posTo) {
			m_matchLen = 0;
			Proceed();
			AfterAction();
			if (m_pProgress)
				m_pProgress->next(m_pos);
		}
		int flagsNext = m_sci.GetFoldLevel(m_line) & ~SC_FOLDLEVELNUMBERMASK;
		m_sci.SetFoldLevel(m_line, m_level | flagsNext);

		delete m_pProgress;
		m_pProgress = NULL;
	}

	void levelIncrease()
	{
		m_levelInc++;
		if (m_levelIncreasedFirst == -1)
			m_levelIncreasedFirst = 1;
	}

	void levelDecrease()
	{
		m_levelDec++;
		if (m_levelIncreasedFirst == -1)
			m_levelIncreasedFirst = 0;
	}

	bool is(char ch)
	{
		if( m_sci.GetCharAt(m_pos)==ch )
			m_matchLen = 1;
		return m_matchLen>0;
	}

	bool is(char ch, char chNext)
	{
		if (m_sci.GetCharAt(m_pos) == ch && m_sci.GetCharAt(m_pos+1) == chNext)
			m_matchLen = 2;
		return m_matchLen>0;
	}

	bool is(const char* sz)
	{
		if( strchr(sz, m_sci.GetCharAt(m_pos)) )
			m_matchLen = 1;
		return m_matchLen>0;
	}

	bool isLink()
	{
		if ( m_sci.GetTextLength() < m_pos + 8 )
			return false;
		string s = m_sci.GetTextRange(m_pos, m_pos + 8);
		int pos = m_pos;
		if (startsWithIC(s, "http://"))
			pos += 7;
		else
		if (startsWithIC(s, "https://"))
			pos += 8;
		else
		if (startsWithIC(s, "www."))
			pos += 4;
		else
			return false;
		for (; true; ++pos) {
			char ch = m_sci.GetCharAt(pos);
			if ( !ch || pos>=m_sci.GetLength() )
				break;
			if( !(std::isdigit(ch) || std::isalpha(ch) || strchr("?&%_.-+=#/:~",ch)) )
				break;
		}
		m_matchLen = pos - m_pos;
		return m_matchLen > 0;
	}

	bool isEOL()
	{
		char ch = m_sci.GetCharAt(m_pos);
		if( ch=='\r' || ch=='\n' ) {
			m_matchLen = 1;
			if( ch=='\r' && m_sci.GetCharAt(m_pos+1)=='\n' )
				m_matchLen = 2;
		}
		return m_matchLen>0;
	}

	bool isDigit()
	{
		char ch = m_sci.GetCharAt(m_pos);
		while( ch>='0' && ch<='9' ) {
			++m_matchLen;
			ch = m_sci.GetCharAt(m_pos+m_matchLen);
		}
		return m_matchLen>0;
	}

	void SetStyle(int nStartPos, int nLength, int nStyle)
	{
		int nEndStyled = m_sci.GetEndStyled();
		if( nStartPos < nEndStyled )
			m_sci.StartStyling(nStartPos, defaultStyleMask);
		else
		if( nStartPos > nEndStyled )
			m_sci.SetStyling(nStartPos-nEndStyled, styleInvalidText);
		m_sci.SetStyling( nLength, nStyle & defaultStyleMask );
	}

	void SetStyle(int nStyle)
	{
		SetStyle(m_pos, m_matchLen, nStyle);
	}

	void SetStyle()
	{
		SetStyle(m_pos, m_matchLen, State2Style(m_state) );
	}

	virtual int State2Style(int nState) = 0;
	virtual void Proceed(void) = 0;
};

class CMyLexerText : public CMyLexerBase {
	enum EnumStates {
		STRING = 1
	};
	enum EnumStyles {
		STYLE_TEXT = 0,
		STYLE_SYMBOL = 1,
		STYLE_NUMBER = 2,
		STYLE_STRING = 3,
//		STYLE_CHAR = 4,
		STYLE_UNQ = 5,
		STYLE_LINK = 6
	};

	bool m_bQuote;
public:
	void setQuote(bool b) { m_bQuote=b; }

	CMyLexerText(CSciWrapper& sci) : CMyLexerBase(sci), m_bQuote(true) { }

	void Proceed(void)
	{
		switch( m_state ) {
			case INITIAL: {
				if( is('"') ) {
					if( m_bQuote ) {
						m_state = STRING;
						SetStyle();
					} else
						SetStyle( STYLE_SYMBOL );
				} else
				if( is("<>(){}[]+-*=&|~`'\\/,.:;%@?!#$^") )
					SetStyle(STYLE_SYMBOL);
				else
				if( isDigit() )
					SetStyle( STYLE_NUMBER );
				else
				if( isLink() )
					SetStyle( STYLE_LINK );
				else {
					m_matchLen = 1;
					SetStyle();
				}
			} break;
			case STRING: {
				if( is('"') ) {
					SetStyle();
					m_state = INITIAL;
				} else
				if( isEOL() ) {
					SetStyle(STYLE_UNQ);
					m_state = INITIAL;
				} else {
					m_matchLen = 1;
					SetStyle();
				}
			} break;
		}
	}

	int State2Style(int nState)
	{
		switch(nState) {
			case STRING: return STYLE_STRING; 
		}
		return STYLE_TEXT;
	}
};

class CMyLexerCMake : public CMyLexerBase {
	enum EnumStates {
		SLCOMMENT = 1
	};
	enum EnumStyles {
		STYLE_TEXT = 0,
		STYLE_COMMENT = 3
	};

public:
	CMyLexerCMake(CSciWrapper& sci) : CMyLexerBase(sci) {}

	void Proceed(void)
	{
		switch( m_state ) {
			case INITIAL: {
				if( is('#') ) {
					m_state = SLCOMMENT;
					SetStyle();
				} else {
					m_matchLen = 1;
					SetStyle();
				}
			} break;
			case SLCOMMENT: {
				if( isEOL() ) {
					SetStyle();
					m_state = INITIAL;
				} else {
					m_matchLen = 1;
					SetStyle();
				}
			} break;
		}
	}

	int State2Style(int nState)
	{
		switch( m_state ) {
			case SLCOMMENT: return STYLE_COMMENT;
		}
		return STYLE_TEXT;
	}
};

class CMyLexerGradle : public CMyLexerBase {
	enum EnumStates {
		SLCOMMENT = 1,
		COMMENT = 2,
		STRING = 3,
		CHAR = 4
	};
	enum EnumStyles {
		STYLE_TEXT = 0,
		STYLE_KW = 1,
		STYLE_COMMENT = 2,
		STYLE_SYMBOL = 3,
		STYLE_NUMBER = 4,
		STYLE_STRING = 5,
		STYLE_CHAR = 6,
		STYLE_UNQ = 7
	};
public:
	CMyLexerGradle(CSciWrapper& sci) : CMyLexerBase(sci) {}

	void Proceed(void)
	{
		switch (m_state) {
		case INITIAL:
		{
			if (is('"')) {
				m_state = STRING;
				SetStyle();
			} else
			if (is('\'')) {
				m_state = CHAR;
				SetStyle();
			} else
			if (is('/','/')) {
				m_state = SLCOMMENT;
				SetStyle();
			} else
			if (is('/','*')) {
				m_state = COMMENT;
				SetStyle();
			} else
			if (is("<>(){}[]+-*=&|~`\\/,.:;%@?!#$^")) {
				char ch = m_sci.GetCharAt(m_pos);
				if (ch == '{') levelIncrease();
				else if (ch == '}') levelDecrease();
				SetStyle(STYLE_SYMBOL);
			} else
			if (isDigit())
				SetStyle(STYLE_NUMBER);
			else {
				m_matchLen = 1;
				SetStyle();
			}
		} break;
		case STRING:
		{
			if (is('"')) {
				SetStyle();
				m_state = INITIAL;
			} else
			if (isEOL()) {
				SetStyle(STYLE_UNQ);
				m_state = INITIAL;
			} else {
				m_matchLen = 1;
				SetStyle();
			}
		} break;
		case CHAR:
		{
			if (is('\'')) {
				SetStyle();
				m_state = INITIAL;
			} else
			if (isEOL()) {
				SetStyle(STYLE_UNQ);
				m_state = INITIAL;
			} else {
				m_matchLen = 1;
				SetStyle();
			}
		} break;
		case SLCOMMENT:
		{
			if ( isEOL() ) {
				SetStyle();
				m_state = INITIAL;
			} else {
				m_matchLen = 1;
				SetStyle();
			}
		} break;
		case COMMENT:
		{
			if ( is( '*', '/' ) ) {
				SetStyle();
				m_state = INITIAL;
			} else {
				m_matchLen = 1;
				SetStyle();
			}
		} break;
		}
	}

	int State2Style(int nState)
	{
		switch (nState) {
			case SLCOMMENT: return STYLE_COMMENT;
			case COMMENT: return STYLE_COMMENT;
			case STRING: return STYLE_STRING;
			case CHAR: return STYLE_CHAR;
		}
		return STYLE_TEXT;
	}
};