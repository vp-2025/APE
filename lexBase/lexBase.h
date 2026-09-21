#pragma once

#include <cassert>
#include <cstdlib>
#include "scintilla.h"
#include "progress.h"
#include <set>
#include <string>
#include "shared/platform.h"
#include "shared/str.h"

using namespace std;

#define SC_FOLDLEVELFOOTERFLAG            (SC_FOLDLEVELHEADERFLAG << 1)

class IStyler {
public:
    enum {
        defaultStyleMask = 0x7F,
        styleInvalidText = 31
    };

    virtual int GetStylingPos() = 0;
    virtual void StartStyling(int posTo) = 0;
    virtual int GetText(char* buf, int max_size) = 0;
    virtual int GetStyle(int nPos) = 0;
    virtual void SetStyle(int nStartPos, int nEndPos, int nStyle) = 0;
    virtual int GetLine(int nPos) = 0;
    virtual void SetLevel(int nLine, int nLevel) = 0;
    virtual int GetLevel(int nLine) = 0;
    virtual int GetLineState(int nLine) = 0;
    virtual void SetLineState(int nLine, int nState) = 0;
};

template<class LEXER>
class CSciLexer : protected LEXER {
    IStyler* m_pStyler;
    int m_langId;

    int m_posSave;
    int m_foldLine;        // line no
    int m_foldLevel;    // fold level
    int m_levelInc;        // fold inc++
    int m_levelDec;        // fold dec--
    int m_levelIncreasedFirst;    // -1 = undef, 1 = inc, 0 = dec

    int MatchStart() { return m_posSave + (this->yytext - this->yy_current_buffer->yy_ch_buf); }
    int MatchEnd() { return MatchStart() + this->yyleng; }
    int MatchLine() { return m_pStyler->GetLine(MatchStart()); }

// level
    void levelIncrease() {
        m_levelInc++;
        if( m_levelIncreasedFirst == -1 )
            m_levelIncreasedFirst = 1;
    }
    void levelDecrease() {
        m_levelDec++;
        if( m_levelIncreasedFirst == -1 )
            m_levelIncreasedFirst = 0;
    }

    int GetLevel() {
        int level = m_foldLevel;
        level &= SC_FOLDLEVELNUMBERMASK;
        level -= SC_FOLDLEVELBASE;
        return level;
    }

    void SetLexerState() {
        int posCur = m_pStyler->GetStylingPos();
        if( posCur <= 0 ) {
            // folding
            m_foldLine = 0;
            m_foldLevel = SC_FOLDLEVELBASE;
            //
            this->yy_start = State2YYState(0);
        } else {
            // folding
            m_foldLine = m_pStyler->GetLine(posCur);
            m_foldLevel = m_pStyler->GetLevel(m_foldLine) & SC_FOLDLEVELNUMBERMASK;
            //
            this->yy_start = m_foldLine > 0 ? m_pStyler->GetLineState(m_foldLine) : State2YYState(0);
        }
        m_levelInc = m_levelDec = 0;
        m_levelIncreasedFirst = -1;
    }


// state <-> yystate
    int State2YYState(int state) { return 1 + 2 * state; }
    int YYState2State(int yystate) { return (yystate - 1) / 2; }
// style <-> yystate
    int YYState2Style(int yystate) { return StateToStyle(YYState2State(yystate)); }

protected:
    virtual int yylex();

    int StateToStyle(int nState);

    void BeforeAction(const char* pszMatchText, int nMatchLength);
    void AfterAction(const char* pszMatchText, int nMatchLength);

    virtual int LexerInput(char* buf, int max_size) {
        m_posSave = m_pStyler->GetStylingPos() - (buf - this->yy_current_buffer->yy_ch_buf);
        return m_pStyler->GetText(buf, max_size);
    }
    virtual void LexerOutput(const char* buf, int size) {}
    virtual void LexerError(const char* msg) {}

    CProgress* m_pProgress;

    void SetStyle(int nStartPos, int nEndPos, int nStyle) {
        m_pStyler->SetStyle(nStartPos, nEndPos, nStyle);
        if( m_pProgress )
            m_pProgress->next(m_pStyler->GetStylingPos());
    }
    void SetStyle(int nStyle) {
        m_pStyler->SetStyle(MatchStart(), MatchEnd(), nStyle);
        if( m_pProgress )
            m_pProgress->next(m_pStyler->GetStylingPos());
    }
    void SetStyle() {
        m_pStyler->SetStyle(MatchStart(), MatchEnd(), YYState2Style(this->yy_start));
        if( m_pProgress )
            m_pProgress->next(m_pStyler->GetStylingPos());
    }

private:
    bool m_bQuote;
public:
    bool isQuote() { return m_bQuote; }
    void setQuote(bool b) { m_bQuote = b; }

    CSciLexer(IStyler* pStyler, int langId = 0) : m_pStyler(pStyler), m_langId(langId), m_pProgress(0),
                                                  m_bQuote(true) {}

    void Style(int posTo) {
        m_pStyler->StartStyling(posTo);
        SetLexerState();
        this->yy_flush_buffer(this->yy_current_buffer);

        if( posTo - m_pStyler->GetStylingPos() > 1000000 )
            m_pProgress = new CProgress("Styling", m_pStyler->GetStylingPos(), posTo);

        while( yylex() != 0 );

        delete m_pProgress;
        m_pProgress = NULL;

        int flagsNext = m_pStyler->GetLevel(m_foldLine) & ~SC_FOLDLEVELNUMBERMASK;
        m_pStyler->SetLevel(m_foldLine, m_foldLevel | flagsNext);
    }
};

#ifdef FLEX_SCANNER
#define YY_USER_ACTION \
        if( yy_act != YY_END_OF_BUFFER ) \
            this->BeforeAction(yytext, yyleng);

#define YY_BREAK \
        this->AfterAction(yytext, yyleng); \
        break;
#endif


// ------------------------------------------------------------------------
// CSciLexer<yyFlexLexer>
// ------------------------------------------------------------------------

template<class LEXER>
void CSciLexer<LEXER>::BeforeAction(const char* pszMatchText, int nMatchLength) {
    if( pszMatchText[nMatchLength - 1] == '\n' ) {
        int foldFlags = 0;
        int levelDiff = m_levelInc - m_levelDec;
        if( levelDiff > 0 )
            foldFlags |= SC_FOLDLEVELHEADERFLAG;
        if( m_levelInc && m_levelDec &&
            m_levelIncreasedFirst == 0 &&
            levelDiff >= 0 ) {
            foldFlags |= SC_FOLDLEVELHEADERFLAG | SC_FOLDLEVELFOOTERFLAG;
            m_foldLevel--;
            levelDiff++;
        }
        m_pStyler->SetLevel(m_foldLine, m_foldLevel | foldFlags);
        m_levelIncreasedFirst = -1;
        m_levelInc = m_levelDec = 0;
        m_foldLevel += levelDiff;
        m_foldLine++;
    }
}

template<class LEXER>
void CSciLexer<LEXER>::AfterAction(const char* pszMatchText, int nMatchLength) {
    if( pszMatchText[nMatchLength - 1] == '\n' ) {
        m_pStyler->SetLineState(m_foldLine, this->yy_start);
    }
}

//////////////////////////////////////////////////////////////////////////////
// keywords
//////////////////////////////////////////////////////////////////////////////

class CKeywords {
protected:
    set<string> st;
    void setText(const string& s, bool bIC);
public:
    CKeywords() {}
    CKeywords(const char* szResID) { setText(LoadTextFromResource(szResID), false); }
    CKeywords(const char* szText, bool) { setText(szText, false); }
    virtual bool is(const char* szText) { return st.find(szText) != st.end(); }
};

class CKeywordsIC : public CKeywords {
public:
    CKeywordsIC(const char* szResID) { setText(LoadTextFromResource(szResID), true); }
    CKeywordsIC(const char* szText, bool) { setText(szText, true); }
    bool is(const char* szText) override { return st.find(toLowerVal(szText)) != st.end(); }
};

void checkPreprocessor(const char* text, int lang, bool& bDec, bool& bInc, bool bIC = false);
