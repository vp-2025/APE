#include "stdafx.h"
#include "export2HTML.h"
#include "options.h"
#include "shared/platform.h"
#include "lexers.h"
#include "lexerStyles.h"
#include "shared/str.h"

string clr2html( int clr )
{
	char buf[20];
	sprintf(buf,"#%02X%02X%02X", GetRValue(clr), GetGValue(clr), GetBValue(clr) );
	return buf;
}

string fontName(const string& s) {
	return string() << " font-family: " << s << ";";
}
string fontSize(int size) {
	return string() << " font-size: " << size << "pt;";
}
string clrBack(int clr) {
	return string() << " background: " << clr2html(clr) <<";";
}
string clrFore(int clr) {
	return string() << " color: " << clr2html(clr) <<";";
}
string fontBold(bool b) {
	return string() << " font-weight: "<< (b?"bold":"normal") <<";";
}
string fontItalic(bool b) {
	return string() << " font-style: "<< (b?"italic":"normal") <<";";
}

void saveStyle( const CLexerStyle& style, string& s ) 
{
    if( !style.IsFont() && 
		!style.IsSize() && 
        !style.IsClrBack() && 
		!style.IsClrFore() &&
        !style.IsBold() && 
		!style.IsItalic() )
        return;

	if( style.id==STYLE_DEFAULT ) s << "span {";
	else s << ".s" <<style.id<<" {";
	if( style.IsFont() ) s << fontName(style.GetFont());
	if( style.IsSize() ) s << fontSize(style.GetSize());
	if( style.IsClrBack() ) s << clrBack(style.GetClrBack());
	if( style.IsClrFore() ) s << clrFore(style.GetClrFore());
	if( style.IsBold() ) s << fontBold( style.GetBold() );
	if( style.IsItalic() ) s << fontItalic( style.GetItalic() );
	s << "}\n";
}

void saveStyle( const CLexerStyle& style1, const CLexerStyle& style2, string& s ) 
{
    if( !style2.IsFont() && 
		!style2.IsSize() && 
        !style2.IsClrBack() && 
		!style2.IsClrFore() &&
        !style2.IsBold() && 
		!style2.IsItalic() &&
		!style1.IsFont() && 
		!style1.IsSize() && 
        !style1.IsClrBack() && 
		!style1.IsClrFore() &&
        !style1.IsBold() && 
		!style1.IsItalic() )
        return;

	if( style2.id==STYLE_DEFAULT ) s << "span {";
	else s << ".s" <<style2.id<<" {";

	if( style2.IsFont() ) s << fontName(style2.GetFont()); 
	else if( style1.IsFont() ) s << fontName(style1.GetFont());

	if( style2.IsSize() ) s << fontSize(style2.GetSize());
	else if( style1.IsSize() ) s << fontSize(style1.GetSize());

	if( style2.IsClrBack() ) s << clrBack(style2.GetClrBack());
	else if( style1.IsClrBack() ) s << clrBack(style1.GetClrBack());

	if( style2.IsClrFore() ) s << clrFore(style2.GetClrFore());
	else if( style1.IsClrFore() ) s << clrFore(style1.GetClrFore());

	if( style2.IsBold() )  s << fontBold(style2.GetBold());
	else if( style1.IsBold() )  s << fontBold(style1.GetBold());

	if( style2.IsItalic() )  s << fontItalic(style2.GetItalic());
	else if( style1.IsItalic() )  s << fontItalic(style1.GetItalic());

	s << "}\n";
}

string saveStyles( CLexerStyle* pStyles ) 
{
	string s;
	for( CLexerStyle* pStyle=pStyles; pStyle->szName; pStyle++ ) 
	{
		if( pStyle->id==STYLE_DEFAULT )
			saveStyle( g_styles[0].styles[0], *pStyle, s ); // default style
		else {
			if( pStyle->parentId ) {
				CLexerStyle* pParent = findStyle(pStyles,pStyle->parentId);
				saveStyle( *pParent, *pStyle, s );
			} else
				saveStyle( *pStyle, s );
		}
	}
	return s;
}

void replaceLeftSpaces( string& str ) 
{
	int spaces = 0;
	int chars = 0;
	for( unsigned k=0; k<str.length(); k++ ) {
		if( str[k]==' ' ) { spaces++; chars++; continue; }
		if( str[k]=='\t' ) { spaces+=g_options.iTabSize; chars++; continue; }
		break;
	}
	if( !chars ) return;
	string strOut;
	for( int k=0; k<spaces; k++ )
		strOut += "&nbsp;";
	strOut += str.substr( chars );
	str = strOut;
}

void encodeStr( string& str ) 
{
    string strOut;
    for( unsigned i=0; i<str.length(); i++ ) {
        char ch = str[i];
        switch( ch ) {
            case '>':   strOut += "&gt;"; break;
            case '<':   strOut += "&lt;"; break;
            case '&':   strOut += "&amp;"; break;
            case '\'':  strOut += "&quot;"; break;
            case '"':   strOut += "&quot;"; break;
            case '`':   strOut += "&apos;"; break;
            default: strOut += ch;
        }
    }
    str = strOut;
}

void proceedStr( CSciWrapper& sci, int p1, string& str ) 
{
	string strOut;
	int prevStyle, style;
	unsigned prevPos=0, pos=0;
	prevStyle = style = sci.GetStyleAt(p1);
	bool bFirst = true;
	for( ; pos<str.length(); pos++ ) { 
		style = sci.GetStyleAt( p1+pos );
		if( style!=prevStyle ) {
			string strMid = str.substr(prevPos,pos-prevPos);
			encodeStr( strMid );
			if( bFirst ) { replaceLeftSpaces(strMid); bFirst = false; }
			strOut << "<span class=s" << prevStyle << ">" << strMid << "</span>";
			prevStyle = style;
			prevPos = pos;
		}
	}
	if( prevPos<pos ) {
		string strMid = str.substr(prevPos,pos-prevPos);
		encodeStr( strMid );
		if( bFirst ) { replaceLeftSpaces(strMid); bFirst = false; }
		strOut << "<span class=s" << style << ">" << strMid << "</span>";
	}
	str = strOut;
}

void export2HTML_int( CSciWrapper& sci, int pos, int len, string& s, const string& sTitle ) 
{
	if( len==-1 )
		len = sci.GetTextLength();
	int pos2 = pos+len;
	sci.ColorizeTo(pos2);

	string css = saveStyles( GetLexerStyles(sci) );

	s.clear();
	s << "<HTML>\n" 
	  << "<HEAD>\n" 
	  << "<meta name=\"GENERATOR\" content=\"APE++\" />\n"
	  << sTitle 
	  << "<style>\n"
	  << css
	  << "</style>\n" 
	  << "</HEAD>\n" 
	  << "<BODY>\n" 
	  << "<!--StartFragment-->\n";

	int line = sci.LineFromPosition(pos);
	int line2 = sci.LineFromPosition(pos2);
	if( line==line2 ) {
		string sLine = sci.GetTextRange( pos, pos2 );
		proceedStr( sci, pos, sLine );
		s << sLine;
	} else {
		int linePos = sci.GetLineStartPosition(line);
		if( pos>linePos ) {
			int p2 = sci.GetLineEndPosition(line);
			string sStartLine = sci.GetTextRange( pos, p2 );
			proceedStr( sci, pos, sStartLine );
			line++;
			s << sStartLine;
			s << "<br/>\n";
		}

		for( int i=line; i<=line2; i++ ) {
			int p = max(sci.GetLineStartPosition(i),pos);
			int p2 = min(sci.GetLineEndPosition(i),pos+len);
			string sLine = sci.GetTextRange( p, p2 );
			proceedStr( sci, p, sLine );
			s << sLine << "<br/>\n";
		}
	}

	s << "<!--EndFragment-->\n" 
	  << "</BODY>\n" 
	  << "</HTML>";
}

void export2HTML( CSciWrapper& sci, const string& sFile ) 
{
	ofstream f;
	f.open( sFile.c_str() );
	if( f.bad() )
	{
		string msg;
		msg << "Error while open file '"<<sFile<<"' for Export to HTML!";
		MsgBoxError( msg );
		MessageBox( sci.hWnd(), msg.c_str(), NULL, MB_OK|MB_ICONSTOP );
		return;
	}
	string sTitle;
	sTitle << "<title>"<< sFile <<"</title>\n";
	string s;
	export2HTML_int( sci, 0, -1, s, sTitle );
	f << s;
}

string export2HTMLSelected( CSciWrapper& sci ) 
{
	int p = sci.GetSelectionStart();
	int p2 = sci.GetSelectionEnd();
	string s;
	export2HTML_int( sci, p, p2-p, s, "" );
	return s;
}

