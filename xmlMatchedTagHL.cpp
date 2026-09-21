#include "stdafx.h"
#include "xmlMatchedTagHL.h"
#include "lexers.h"
#include "shared/str.h"

int CXmlMatchedTagsHighlighter::getFirstTokenPosFrom(int targetStart, int targetEnd, const string& token, pair<int, int> & foundPos)
{
	_pEditView->Call(SCI_SETTARGETSTART, targetStart);
	_pEditView->Call(SCI_SETTARGETEND, targetEnd);
	_pEditView->Call(SCI_SETSEARCHFLAGS, SCFIND_REGEXP|SCFIND_POSIX);
	int posFind = _pEditView->Call(SCI_SEARCHINTARGET, token.length(), (int)token.c_str());
	if (posFind != -1)
	{
		foundPos.first = _pEditView->Call(SCI_GETTARGETSTART);
		foundPos.second = _pEditView->Call(SCI_GETTARGETEND);
	}
	return posFind;
}

TagCateg CXmlMatchedTagsHighlighter::getTagCategory(XmlMatchedTagsPos & tagsPos, int curPos)
{
	pair<int, int> foundPos;

	int docLen = _pEditView->GetLength();

	int gtPos = getFirstTokenPosFrom(curPos, 0, ">", foundPos);
	int ltPos = getFirstTokenPosFrom(curPos, 0, "<", foundPos);
	if (ltPos != -1)
	{
		if ((gtPos != -1) && (ltPos < gtPos))
			return outOfTag;

		// Now we are sure about that we are inside of tag
		// We'll try to determinate the tag category :
		// tagOpen : <Tag>, <Tag Attr="1" >
		// tagClose : </Tag>
		// tagSigle : <Tag/>, <Tag Attr="0" />
		int charAfterLt = _pEditView->Call(SCI_GETCHARAT, ltPos+1);
		if (!charAfterLt)
			return unknownPb;

		if ((char)charAfterLt == ' ')
			return invalidTag;

		// so now we are sure we have tag sign '<'
		// We'll see on the right
		int gtPosOnR = getFirstTokenPosFrom(curPos, docLen, ">", foundPos);
		int ltPosOnR = getFirstTokenPosFrom(curPos, docLen, "<", foundPos);

		if (gtPosOnR == -1)
			return invalidTag;

		if ((ltPosOnR != -1) && (ltPosOnR < gtPosOnR))
			return invalidTag;

		if ((char)charAfterLt == '/')
		{
			int char2AfterLt = _pEditView->Call(SCI_GETCHARAT, ltPos+1+1);

			if (!char2AfterLt)
				return unknownPb;

			if ((char)char2AfterLt == ' ')
				return invalidTag;

			tagsPos.tagCloseStart = ltPos;
			tagsPos.tagCloseEnd = gtPosOnR + 1;
			return tagClose;
		}
		else
		{
			// it's sure for not being a tagClose
			// So we determinate if it's tagSingle or tagOpen
			tagsPos.tagOpenStart = ltPos;
			tagsPos.tagOpenEnd = gtPosOnR + 1;

			int charBeforeLt = _pEditView->Call(SCI_GETCHARAT, gtPosOnR-1);
			if ((char)charBeforeLt == '/')
				return inSingleTag;

			return tagOpen;
		}
	}
		
	return outOfTag;
}

bool CXmlMatchedTagsHighlighter::getMatchedTagPos(int searchStart, int searchEnd, const string& tag2find, const string& oppositeTag2find, vector<int> oppositeTagFound, XmlMatchedTagsPos & tagsPos)
{
	const bool search2Left = false;
	const bool search2Right = true;

	bool direction = searchEnd > searchStart;

	pair<int, int> foundPos;
	int ltPosOnR = getFirstTokenPosFrom(searchStart, searchEnd, tag2find, foundPos);
	if (ltPosOnR == -1)
		return false;

	// if the tag is found in non html zone, we skip it
/*	const NppGUI & nppGUI = (NppParameters::getInstance())->getNppGUI();
	int idStyle = _pEditView->Call(SCI_GETSTYLEAT, ltPosOnR);
	if (!nppGUI._enableHiliteNonHTMLZone && (idStyle >= SCE_HJ_START || idStyle == SCE_H_COMMENT))
	{
		int start = (direction == search2Left)?foundPos.first:foundPos.second;
		int end = searchEnd;
		return getMatchedTagPos(start, end, tag2find, oppositeTag2find, oppositeTagFound, tagsPos);
	}*/

	TagCateg tc = outOfTag;
	if (direction == search2Left)
	{
		tc = getTagCategory(tagsPos, ltPosOnR+2);
		
		if (tc != tagOpen && tc != inSingleTag)
 			return false;
		if (tc == inSingleTag)
		{
			int start = foundPos.first;
			int end = searchEnd;
			return getMatchedTagPos(start, end, tag2find, oppositeTag2find, oppositeTagFound, tagsPos);
		}
	}

	pair<int, int> oppositeTagPos;
	int s = foundPos.first;
	int e = tagsPos.tagOpenEnd;
	if (direction == search2Left)
	{
		s = foundPos.second;
		e = tagsPos.tagCloseStart;
	}

	int ltTag = getFirstTokenPosFrom(s, e, oppositeTag2find, oppositeTagPos);

	if (ltTag == -1)
	{
		if (direction == search2Left)
		{
			return true;
		}
		else
		{
			tagsPos.tagCloseStart = foundPos.first;
			tagsPos.tagCloseEnd = foundPos.second;
			return true;
		}
	}
	else 
	{
		// RegExpr is "<tagName[ 	>]", found tag could be a openTag or singleTag
		// so we should make sure if it's a singleTag
		XmlMatchedTagsPos pos;
		if (direction == search2Right && getTagCategory(pos,ltTag+1) == inSingleTag)
		{
			for(;;)
			{
				ltTag = getFirstTokenPosFrom(ltTag, e, oppositeTag2find, oppositeTagPos);
				
				if (ltTag == -1)
				{
					tagsPos.tagCloseStart = foundPos.first;
					tagsPos.tagCloseEnd = foundPos.second;
					return true;
				}
				else 
				{
					if (getTagCategory(pos,ltTag+1) == inSingleTag)
					{
						continue;
					}

					if (!isInList(ltTag, oppositeTagFound))
					{
						oppositeTagFound.push_back(ltTag);
						break;
					}
				}
			}
			return getMatchedTagPos(foundPos.second, searchEnd, tag2find, oppositeTag2find, oppositeTagFound, tagsPos);
		}


		if (isInList(ltTag, oppositeTagFound))
		{
			for(;;)
			{
				ltTag = getFirstTokenPosFrom(ltTag, e, oppositeTag2find, oppositeTagPos);
				if (ltTag == -1)
				{
					if (direction == search2Left)
					{
						return true;
					}
					else
					{
						tagsPos.tagCloseStart = foundPos.first;
						tagsPos.tagCloseEnd = foundPos.second;
					}
					return true;
				}
				else if (!isInList(ltTag, oppositeTagFound))
				{
					oppositeTagFound.push_back(ltTag);
					break;
				}
				else
				{
					if (direction == search2Left)
					{
						XmlMatchedTagsPos tmpTagsPos;
						getTagCategory(tmpTagsPos, ltTag+1);
						ltTag = tmpTagsPos.tagCloseEnd;
					}
				}
			}
		}
		else
		{
			oppositeTagFound.push_back(ltTag);
		}
	}
	int start, end;
	if (direction == search2Left)
	{
		start = foundPos.first;
		end = searchEnd;
	}
	else
	{
		start = foundPos.second;
		end = searchEnd;
	}

	return getMatchedTagPos(start, end, tag2find, oppositeTag2find, oppositeTagFound, tagsPos);
}


bool CXmlMatchedTagsHighlighter::getXmlMatchedTagsPos(XmlMatchedTagsPos & tagsPos)
{
	// get word where caret is on
	int caretPos = _pEditView->Call(SCI_GETCURRENTPOS);
	
	// if the tag is found in non html zone (include comment zone), then quit
/*	const NppGUI & nppGUI = (NppParameters::getInstance())->getNppGUI();
	int idStyle = _pEditView->Call(SCI_GETSTYLEAT, caretPos);
	if (!nppGUI._enableHiliteNonHTMLZone && (idStyle >= SCE_HJ_START || idStyle == SCE_H_COMMENT))
		return false;*/

	int docLen = _pEditView->GetLength();

	// determinate the nature of current word : tagOpen, tagClose or outOfTag
	TagCateg tagCateg = getTagCategory(tagsPos, caretPos);

	string sWordChars = _pEditView->GetWordChars();
	_pEditView->SetWordChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_:");
	bool ret=false;
	switch (tagCateg)
	{
		case tagOpen : // if tagOpen search right
		{
			int startPos = _pEditView->Call(SCI_WORDSTARTPOSITION, tagsPos.tagOpenStart+1, true);
			int endPos = _pEditView->Call(SCI_WORDENDPOSITION, tagsPos.tagOpenStart+1, true);
			tagsPos.tagNameEnd = endPos;

			string tagName = _pEditView->GetTextRange(startPos, endPos);
			string openTag, closeTag;
			openTag << "<" << tagName << "[ \t>]+";
			closeTag << "</" << tagName << ">";

			vector<int> passedTagList;
			ret = getMatchedTagPos(tagsPos.tagOpenEnd, docLen, closeTag, openTag, passedTagList, tagsPos);
			break;
		}

		case tagClose : // if tagClose search left
		{
			int startPos = _pEditView->Call(SCI_WORDSTARTPOSITION, tagsPos.tagCloseStart+2, true);
			int endPos = _pEditView->Call(SCI_WORDENDPOSITION, tagsPos.tagCloseStart+2, true);

			string tagName = _pEditView->GetTextRange(startPos, endPos);
			string openTag, closeTag;
			openTag << "<" << tagName << "[ \t>]+";
			closeTag << "</" << tagName << ">";
			
			vector<int> passedTagList;
			bool isFound = getMatchedTagPos(tagsPos.tagCloseStart, 0, openTag, closeTag, passedTagList, tagsPos);
			if (isFound)
				tagsPos.tagNameEnd = tagsPos.tagOpenStart + 1 + (endPos - startPos);

			ret = isFound;
			break;
		}

		case inSingleTag : // if in single tag
		{
			int endPos = _pEditView->Call(SCI_WORDENDPOSITION, tagsPos.tagOpenStart+1, true);
			tagsPos.tagNameEnd = endPos;

			tagsPos.tagCloseStart = -1;
			tagsPos.tagCloseEnd = -1;
			ret = true;
		}		
	}
	_pEditView->SetWordChars( sWordChars );
	return ret; // if outOfTag, just quit
}

vector< pair<int, int> > CXmlMatchedTagsHighlighter::getAttributesPos(int start, int end)
{
	vector< pair<int, int> > attributes;

	string buf = _pEditView->GetTextRange(start, end);

	enum {
		attr_invalid,
		attr_key,
		attr_pre_assign,
		attr_assign,
		attr_string,
		attr_value,
		attr_valid
	} state = attr_invalid;

	int startPos = -1;
	int oneMoreChar = 1;
	size_t i = 0;
	for (; i < buf.length(); i++)
	{
		switch (buf[i])
		{
			case ' ':
			case '\t':
			case '\n':
			case '\r':
			{
				if (state == attr_key)
					state = attr_pre_assign;
				else if (state == attr_value)
				{
					state = attr_valid;
					oneMoreChar = 0;
				}
			}
			break;

			case '=':
			{
				if (state == attr_key || state == attr_pre_assign)
					state = attr_assign;
				else if (state == attr_assign || state == attr_value)
					state = attr_invalid;
			}
			break;

			case '"':
			{
				if (state == attr_string)
				{
					state = attr_valid;
					oneMoreChar = 1;
				}
				else if (state == attr_key || state == attr_pre_assign || state == attr_value)
					state = attr_invalid;
				else if (state == attr_assign)
					state = attr_string;
			}
			break;

			default:
			{
				if (state == attr_invalid)
				{
					state = attr_key;
					startPos = i;
				}
				else if (state == attr_pre_assign)
					state = attr_invalid;
				else if (state == attr_assign)
					state = attr_value;
			}
		}

		if (state == attr_valid)
		{
			attributes.push_back(pair<int, int>(start+startPos, start+i+oneMoreChar));
			state = attr_invalid;
		}
	}
	if (state == attr_value)
		attributes.push_back(pair<int, int>(start+startPos, start+i-1));

	return attributes;
}

void CXmlMatchedTagsHighlighter::tagClear(CSciWrapper* pSci) 
{
	_pEditView = pSci;
	_pEditView->clearIndicator(INDIC_TAGMATCH);
	_pEditView->clearIndicator(INDIC_TAGATTR);
}

void CXmlMatchedTagsHighlighter::tagHighlight(CSciWrapper* pSci, bool doHiliteAttr) 
{
	// Detect the current lang type. It works only with html and xml
	if( pSci->Lexer()!=LEX_XHTML && pSci->Lexer()!=LEX_XML )
		return;

	// if move inside tag then skip it
	if( m_xmlTags.isInside(pSci->GetCurrentPos()) )
		return;

	tagClear(pSci);

	// Get the original targets and search options to restore after tag matching operation
	int originalStartPos = _pEditView->Call(SCI_GETTARGETSTART);
	int originalEndPos = _pEditView->Call(SCI_GETTARGETEND);
	int originalSearchFlags = _pEditView->Call(SCI_GETSEARCHFLAGS);

    // Detect if it's a xml/html tag. If yes, Colour it!
	bool b = getXmlMatchedTagsPos(m_xmlTags);
	if( !b )
		m_xmlTags.reset();

	// restore the original targets and search options to avoid the conflit with search/replace function
	_pEditView->Call(SCI_SETTARGETSTART, originalStartPos);
	_pEditView->Call(SCI_SETTARGETEND, originalEndPos);
	_pEditView->Call(SCI_SETSEARCHFLAGS, originalSearchFlags);

	if( b )	{
		_pEditView->Call(SCI_SETINDICATORCURRENT, INDIC_TAGMATCH);
		int openTagTailLen = 2;

		// Colourising the close tag firstly
		if ((m_xmlTags.tagCloseStart != -1) && (m_xmlTags.tagCloseEnd != -1))
		{
			_pEditView->Call(SCI_INDICATORFILLRANGE,  m_xmlTags.tagCloseStart, m_xmlTags.tagCloseEnd - m_xmlTags.tagCloseStart);
			// tag close is present, so it's not single tag
			openTagTailLen = 1;
		}

		// Colourising the open tag
		_pEditView->Call(SCI_INDICATORFILLRANGE,  m_xmlTags.tagOpenStart, m_xmlTags.tagNameEnd - m_xmlTags.tagOpenStart);
		_pEditView->Call(SCI_INDICATORFILLRANGE,  m_xmlTags.tagOpenEnd - openTagTailLen, openTagTailLen);

        
        // Colouising its attributs
        if (doHiliteAttr)
		{
			vector< pair<int, int> > attributes = getAttributesPos(m_xmlTags.tagNameEnd, m_xmlTags.tagOpenEnd - openTagTailLen);
			_pEditView->Call(SCI_SETINDICATORCURRENT,  INDIC_TAGATTR);
			for (size_t i = 0 ; i < attributes.size() ; i++)
			{
				_pEditView->Call(SCI_INDICATORFILLRANGE,  attributes[i].first, attributes[i].second - attributes[i].first);
			}
        }

        // Colouising indent guide line position
/*		if (_pEditView->isShownIndentGuide())
		{
			int columnAtCaret  = int(_pEditView->Call(SCI_GETCOLUMN, xmlTags.tagOpenStart));
			int columnOpposite = int(_pEditView->Call(SCI_GETCOLUMN, xmlTags.tagCloseStart));

			int lineAtCaret  = int(_pEditView->Call(SCI_LINEFROMPOSITION, xmlTags.tagOpenStart));
			int lineOpposite = int(_pEditView->Call(SCI_LINEFROMPOSITION, xmlTags.tagCloseStart));

			if (xmlTags.tagCloseStart != -1 && lineAtCaret != lineOpposite)
			{
				_pEditView->Call(SCI_BRACEHIGHLIGHT, xmlTags.tagOpenStart, xmlTags.tagCloseEnd-1);
				_pEditView->Call(SCI_SETHIGHLIGHTGUIDE, (columnAtCaret < columnOpposite)?columnAtCaret:columnOpposite);
			}
		}*/
	}
}
