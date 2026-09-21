#pragma once

#include "sci.h"
#include "shared/win.h" // CRegistry

enum {
	LEX_NONE=1,
	LEX_SEPARATOR,
	LEX_CPP,
	LEX_C,
	LEX_CS,
	LEX_IDL,
	LEX_RC,
	LEX_JAVA,
	LEX_JS,
	LEX_PROTO,
	LEX_XHTML,
	LEX_XML,
	LEX_CSS,
	LEX_PASCAL,
	LEX_CONF,
	LEX_INI,
	LEX_TXT,
	LEX_TXT_NOQUOTE,
	LEX_SQL,
	LEX_BATCH,
	LEX_NSIS,
	LEX_MAKE,
	LEX_CMAKE,
	LEX_GRADLE,
	LEX_MSBUILD
};

struct CLexerInfo {
	int iLexerID;
	const char* szLexerName;
	const char* szSourceMasks[10];
	const char* szIncludeMasks[10];
};
extern CLexerInfo g_lexers[];

struct CLexerStyle {
	int id;					// style id
	const char* szName;		// style name
	union {
		int iMask;			// mask
		struct {
			unsigned isBold:1;
			unsigned bBold:1;
			unsigned isItalic:1;
			unsigned bItalic:1;
			unsigned isUnderline:1;
			unsigned bUnderline:1;
			unsigned isEOL:1;
			unsigned bEOL:1;
			unsigned isHotSpot:1;
			unsigned bHotSpot:1;

			unsigned isSize:1;
			unsigned isFont:1;
			unsigned isFore:1;
			unsigned isBack:1;
		};
	};
	int clrFore;			// foregrounf color
	int clrBack;			// background color
	int iSize;				// font size
	string sFont;			// font name
	int parentId;			// parent style id
	const char* szDesc;		// style description

	int iMaskDef, clrForeDef, clrBackDef, iSizeDef;
	string sFontDef;

	bool IsBold() const { return isBold; }
	bool IsItalic() const { return isItalic; }
	bool IsUnderline() const { return isUnderline; }
	bool IsEOL() const { return isEOL; }

	bool GetBold() const { return bBold; }
	bool GetItalic() const { return bItalic; }
	bool GetUnderline() const { return bUnderline; }
	bool GetEOL() const { return bEOL; }

	bool IsSize() const { return isSize; }
	bool IsFont() const { return isFont; }
	bool IsClrFore() const { return isFore; }
	bool IsClrBack() const { return isBack; }

	int GetSize() const { return iSize; }
	const string& GetFont() const { return sFont; }
	int GetClrFore() const { return clrFore; }
	int GetClrBack() const { return clrBack; }

	bool diffMask() const { return iMask!=iMaskDef; }
	bool diffSize() const { return isSize && iSize!=iSizeDef; }
	bool diffFont() const { return isFont && sFont!=sFontDef; }
	bool diffFore() const { return isFore && clrFore!=clrForeDef; }
	bool diffBack() const { return isBack && clrBack!=clrBackDef; }
	bool diff() const { return diffMask() || diffSize() || diffFont() || diffFore() || diffBack(); }

	void Init();
	void Load( CRegistry& reg, string key );
	bool Write( CRegistry& reg, string key ) const;
};

struct CLexerStyles {
	const char* szName, *szID;
	CLexerStyle* styles;
};

extern CLexerStyles g_styles[];

struct CIndicator {
	int id;
	const char* szName;
	int color, alpha, outlineAlpha, style;

	int colorDef, alphaDef, outlineAlphaDef, styleDef;

	bool diffColor() const { return color!=colorDef; }
	bool diffAlpha() const { return alpha!=alphaDef; }
	bool diffOutlineAlpha() const { return outlineAlpha!=outlineAlphaDef; }
	bool diffStyle() const { return style!=styleDef; }
	bool diff() const { return diffColor() || diffAlpha() || diffOutlineAlpha() || diffStyle(); }

	void Init();
	void Load( CRegistry& reg );
	bool Write( CRegistry& reg ) const;
};

extern CIndicator g_indicators[];

int DetectLexer( const string& sFile );
void onStyleNeeded( CSciWrapper& sci, const string& sFile, int posTo );
string doAlternate( const string& sFile );

void LoadLexerStyles();
void WriteLexerStyles();

CLexerStyle* findStyle( CLexerStyle* pStyles, int id );