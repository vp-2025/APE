#pragma once

#include <windows.h>
#include <string>
#include <map>

std::wstring accel2str( BYTE virt, WORD key ); // "Ctrl+Shift+C" style name of a single accelerator key

class CAccel2Menu {
	typedef std::map<int,std::wstring> mAccelType;
	mAccelType m;
public:
	CAccel2Menu( UINT nAccelID );
	void proceedMenu( HMENU hMenu );
	void proceedMenu( HWND hWnd ) { proceedMenu(GetMenu(hWnd)); }
};
