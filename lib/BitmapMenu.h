#pragma once

#include <windows.h>
#include <commctrl.h>
#include <map>
#include <vector>
#include "shared/win.h" // CPoint & CSize

struct CToolbarInfo
{
	std::map<int,int> mCmd2Idx;
	HIMAGELIST hIL;
};

typedef bool (* PFN_OnGetImage)(void* pObj, UINT menuID, HIMAGELIST& hIL, int& imageID);

class CBitmapMenu
{
	std::vector<CToolbarInfo> m_toolbars;
	void* m_pObj; 
	PFN_OnGetImage m_OnGetImage;
public:
	CBitmapMenu() { m_pObj=nullptr; m_OnGetImage=nullptr; }
	bool AddToolbar( int );
	void OnInitMenuPopup( HMENU );
	void OnMeasureItem( MEASUREITEMSTRUCT* );
	void OnDrawItem( DRAWITEMSTRUCT* );
	void SetGetImage( void* pObj, PFN_OnGetImage OnGetImage ) { m_pObj=pObj; m_OnGetImage=OnGetImage; }
private:
	void GetImage( UINT menuID, HIMAGELIST& hIL, int& imageID );
	void DrawButton( HDC dc, CSize& buttonSize );
	void DrawChecked(HDC dc, CSize& buttonSize);
	void DrawDisabled( HDC dc, HIMAGELIST& hIL, int imageID, CPoint& position, CSize& size );
};