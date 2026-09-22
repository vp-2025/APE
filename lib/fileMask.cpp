#include "fileMask.h"
#include "shared/str.h"
#include <regex>
#include <map>

using namespace std;

bool isFileMask(string sMask, string sFile) {
	replaceStr(sMask, ".", "[.]");
	replaceStr(sMask, "*", ".*");
	replaceStr(sMask, "?", ".");

	// Building a std::regex is expensive and DetectLexer walks the same mask
	// tables for every file, so keep the compiled patterns around
	static map<string,std::regex> cache;
	auto it = cache.find(sMask);
	if( it == cache.end() )
		it = cache.emplace(sMask, std::regex(sMask)).first;

	toLower(sFile);
	return std::regex_match(sFile, it->second);
}