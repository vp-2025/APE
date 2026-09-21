#include "str.h"

using namespace std;

void testShared() {
    string sU = u8"тест ÄÖÜäöüß";
    wstring sW = utf2w(sU);
    string sA = utf2a(sU);
    string sWU = w2utf(sW);
    string sAU = a2utf(sA);
}

