#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include "shared/ctime.h"
#include "shared/platform.h"
#include "sci.h"

struct CFuncInfo {
    int lineStart, lineEnd;
    int posStart, posEnd, posName;
    string name, params;
    CFuncInfo( const string& nm, const string& prm, int line1, int line2, int pos1, int pos2, int pos3 )
    { name=nm; params=prm; lineStart=line1; lineEnd=line2; posStart=pos1; posEnd=pos2; posName=pos3; }
    bool in(int pos) const { return posStart<=pos && pos<=posEnd; }
    bool after(int pos) const { return posStart>pos || posEnd>pos; }
};

inline bool cmpByLine( const CFuncInfo& f1, const CFuncInfo& f2 ) { return f1.posStart<f2.posEnd; }
inline bool cmpByName( const CFuncInfo& f1, const CFuncInfo& f2 ) { return strcmpi(f1.name.c_str(),f2.name.c_str())<0; } // f1.name<f2.name;

class CFuncs : public vector<CFuncInfo> {
public:
    void sortByLine() { sort(this->begin(),this->end(),cmpByLine); }
    void sortByName() { sort(this->begin(),this->end(),cmpByName); }
};

struct CTabPage {
    bool bNew;		// file is new (untitled)
    bool bExist;	// file exists of disk: notNew and notDeleted
    string sFile;
    // icon
    int64_t size;
    CTime tm;
    CSciWrapper sci;
    CFuncs vFunc;

    CTabPage() { bNew=bExist=false; size=0;	}
    bool loadFromFile() { return bExist=sci.loadFromFile(sFile, tm, size, true); }
    bool saveToFile() { return sci.saveToFile(sFile, tm, size); }
    bool IsFileExists() { return bExist=::IsFileExists(sFile); }
    void reloadSizeAndTime() { if( IsFileExists() ) { tm=vGetFileTime(sFile); size=vGetFileSize(sFile); } }
};
