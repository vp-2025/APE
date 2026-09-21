#pragma once

#include <windows.h>
#include <string>
#include <map>

class CAccel2Menu
{
	typedef std::map<int,std::wstring> mAccelType;
	mAccelType m;
public:
	CAccel2Menu( UINT nAccelID );
	void proceedMenu( HMENU hMenu );
	void proceedMenu( HWND hWnd ) { proceedMenu(GetMenu(hWnd)); }
};
