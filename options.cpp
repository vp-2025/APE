#include "stdafx.h"
#include "options.h"
#include "shared/win.h"

COptions g_options;

#define OPT "Options"
void COptions::ACX(bool bSave)
{
	acx( OPT, "OneInstance", bOneInstanse, bSave );
	acx( OPT, "CheckExternalModify", bCheckExternalModify, bSave );
//	acx( OPT, "SaveOnDeactivate", bSaveOnDeactivate, bSave );
	acx( OPT, "AutoIndent", bAutoIndent, bSave );
//	acx( OPT, "StripTrailingSpaces", bStripTrailingSpaces, bSave );
	acx( OPT, "TabSize", iTabSize, bSave );

	acx( OPT, "TabsCloseBtn", bTabsCloseBtn, bSave );
	acx( OPT, "TabsDragNDrop", bTabsDragNDrop, bSave );

	acx( OPT, "ViewWS", iViewWS, bSave );
	acx( OPT, "ViewEOL", bViewEOL, bSave );
	acx( OPT, "ViewBraceHL", bViewBraceHL, bSave );
	acx( OPT, "ViewNums", bViewNums, bSave );
	acx( OPT, "ViewIndentGuides", bViewIndentGuides, bSave );
	acx( OPT, "ViewLineHL", bViewLineHL, bSave );
	acx( OPT, "ViewXMLMatchTagHL", bViewXMLMatchTagHL, bSave );
	acx( OPT, "ViewTokenHL", bViewTokenHL, bSave );
	acx( OPT, "ViewWrapLongLines", bViewWrapLongLines, bSave );
}