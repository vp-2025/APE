#include "fileMask.h"

void testLib() {
	bool b1 = isFileMask("*.cpp", "file.cpp");
	bool b2 = isFileMask("*.h", "file.h");
	bool bA = isFileMask("a*.c??", "aOne.cpp");
	bool bB = isFileMask("a*.h??", "aOne.hpp");
	bool bC = isFileMask("a*.c??", "aOne.cp");
	bool bD = isFileMask("a*.h??", "aOne.hp");
}