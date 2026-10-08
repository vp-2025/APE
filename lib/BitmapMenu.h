#pragma once

#include <windows.h>
#include <commctrl.h>
#include <map>
#include <tuple>
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
	std::map<std::tuple<HIMAGELIST,int,bool>, HBITMAP> m_bitmaps; // dark mode: themed menu bitmaps
public:
	CBitmapMenu() { m_pObj=nullptr; m_OnGetImage=nullptr; }
	~CBitmapMenu() {
		for( auto& it: m_bitmaps )
			DeleteObject(it.second);
	}
	bool AddToolbar( int );
	void OnInitMenuPopup( HMENU );
	void OnMeasureItem( MEASUREITEMSTRUCT* );
	void OnDrawItem( DRAWITEMSTRUCT* );
	void SetGetImage( void* pObj, PFN_OnGetImage OnGetImage ) { m_pObj=pObj; m_OnGetImage=OnGetImage; }
private:
	void GetImage( UINT menuID, HIMAGELIST& hIL, int& imageID );
	HBITMAP GetBitmap( HIMAGELIST hIL, int imageID, bool bDisabled );
	void DrawButton( HDC dc, CSize& buttonSize );
	void DrawChecked(HDC dc, CSize& buttonSize);
	void DrawDisabled( HDC dc, HIMAGELIST& hIL, int imageID, CPoint& position, CSize& size );
};