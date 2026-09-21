#pragma once

#include <string>
#include "shared/str.h"

class CVersionInfo {
	strbuf m_sBuf;
	std::string m_strInfo;

    std::string getInfoString( const char* sz );
public:
	explicit CVersionInfo( std::string sFileName="" );

    std::string getFileVersion()	 { return getInfoString("FileVersion"); }
    std::string getFileDescription() { return getInfoString("FileDescription"); }
    std::string getComments()		 { return getInfoString("Comments"); }
    std::string getCompanyName()	 { return getInfoString("CompanyName"); }
    std::string getLegalCopyright()  { return getInfoString("LegalCopyright"); }
    std::string getProductName()	 { return getInfoString("ProductName"); }
    std::string getProductVersion()  { return getInfoString("ProductVersion"); }
};