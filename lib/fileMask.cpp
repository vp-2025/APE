#include "fileMask.h"
#include "shared/str.h"
#include <boost/regex.hpp>

using namespace std;

bool isFileMask(string sMask, string sFile) {
	replaceStr(sMask, ".", "[.]");
	replaceStr(sMask, "*", ".*");
	replaceStr(sMask, "?", ".");

	toLower(sFile);
	boost::regex re(sMask);
	return boost::regex_match(sFile, re, boost::match_default);
}