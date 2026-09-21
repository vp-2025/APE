#include "str.h"

using namespace std;

void testShared() {
    string sU = (const char*)u8"тест ÄÖÜäöüß"; // char8_t* in c++20
    wstring sW = utf2w(sU);
    string sA = utf2a(sU);
    string sWU = w2utf(sW);
    string sAU = a2utf(sA);
}

