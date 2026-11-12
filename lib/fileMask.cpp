#include "fileMask.h"

using namespace std;

static char lowerAscii(char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; }

// * any chars, ? one char, case-insensitive (ASCII); on a mismatch only the last *
// takes one more char, so no recursion and at worst O(mask * file)
// iLastStar: the last * seen in sMask, iFileAtStar: where in sFile that * stops for now
bool isFileMask(const string& sMask, const string& sFile) {
	size_t iMask = 0, iFile = 0;
	size_t iLastStar = string::npos, iFileAtStar = 0;
	while( iFile < sFile.size() ) {
		if( iMask < sMask.size() && sMask[iMask] == '*' ) {
			iLastStar = iMask++;
			iFileAtStar = iFile;
		} else if( iMask < sMask.size() && (sMask[iMask] == '?' || lowerAscii(sMask[iMask]) == lowerAscii(sFile[iFile])) ) {
			iMask++;
			iFile++;
		} else if( iLastStar != string::npos ) {
			iMask = iLastStar + 1;
			iFile = ++iFileAtStar;
		} else
			return false;
	}
	while( iMask < sMask.size() && sMask[iMask] == '*' )
		iMask++;
	return iMask == sMask.size();
}