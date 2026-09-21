#pragma once

#include <string>
#include <vector>

class CSciWrapper;

enum TagCateg {tagOpen, tagClose, inSingleTag, outOfTag, invalidTag, unknownPb};

class CXmlMatchedTagsHighlighter {
	CSciWrapper* _pEditView;
public:
	void tagClear(CSciWrapper* pSci);
	void tagHighlight(CSciWrapper* pSci, bool doHiliteAttr);
private:
	struct XmlMatchedTagsPos {
		int tagOpenStart;
		int tagNameEnd;
		int tagOpenEnd;

		int tagCloseStart;
		int tagCloseEnd;
		XmlMatchedTagsPos() { reset(); }
		void reset() { tagOpenStart=tagNameEnd=tagOpenEnd=tagCloseStart=tagCloseEnd=-1; }
		bool isInside(int pos) { return (pos>tagOpenStart && pos<tagOpenEnd) || (pos>tagCloseStart && pos<tagCloseEnd); }
	};
	XmlMatchedTagsPos m_xmlTags;
	
	int getFirstTokenPosFrom(int targetStart, int targetEnd, const std::string& token, std::pair<int, int> & foundPos);
	TagCateg getTagCategory(XmlMatchedTagsPos & tagsPos, int curPos);
	bool getMatchedTagPos(int searchStart, int searchEnd, const std::string& tag2find, const std::string& oppositeTag2find, std::vector<int> oppositeTagFound, XmlMatchedTagsPos & tagsPos);
	bool getXmlMatchedTagsPos(XmlMatchedTagsPos & tagsPos);
	std::vector< std::pair<int, int> > getAttributesPos(int start, int end);
	bool isInList(int element, const std::vector<int>& elementList) {
		for(int el : elementList)
			if( element==el )
				return true;
		return false;
	};
};