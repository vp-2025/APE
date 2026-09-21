#pragma once

#include <string>
#include <map>

struct CItemNSIS 
{
	std::string sParams;
	std::string sDesc;

	CItemNSIS() {}
	CItemNSIS( const std::string& sParams ) { this->sParams = sParams; }
};

struct cmpStringIC 
{
	bool operator() ( const std::string& s1, const std::string& s2 ) const
	{
		return strcmpi( s1.c_str(), s2.c_str())<0 ; 
	}
};

typedef std::map< std::string, CItemNSIS, cmpStringIC > CMapNSIS;

extern CMapNSIS g_mapNSIS;
extern std::string g_sApiNSIS;

void initNSIS();
bool isKeywordNSIS( const char* szKW );