#pragma once

struct COptions
{
	bool bOneInstanse;
	bool bCheckExternalModify;
	bool bSaveOnDeactivate;
	int iTabSize;
	bool bAutoIndent;
	bool bStripTrailingSpaces;
	int iMRUSize;

	bool bTabsCloseBtn, bTabsDragNDrop;

	int iViewWS;
	bool bViewEOL, bViewBraceHL, bViewNums, bViewIndentGuides, bViewLineHL, bViewXMLMatchTagHL, bViewWrapLongLines, bViewTokenHL;

	COptions()
	{
		bOneInstanse = true;
		bCheckExternalModify = true;
		bStripTrailingSpaces = bSaveOnDeactivate = false;
		iTabSize = 4;
		bAutoIndent = true;
		iMRUSize = 20;
		bTabsCloseBtn=bTabsDragNDrop=true;

		iViewWS = 0;
		bViewEOL = bViewNums = bViewIndentGuides = bViewWrapLongLines = false;
		bViewBraceHL = bViewLineHL = bViewXMLMatchTagHL = bViewTokenHL = true;

		Load();
	}
	void Load() { ACX(false); }
	void Save() { ACX(true); }
private:
	void ACX(bool bSave);
};

extern COptions g_options;