#include "BitmapMenu.h"
#include "resource.h" // for ID_TAB_CHECKED
using namespace std;

extern HINSTANCE g_hInst;

#define TEXT_SPACE 3

// structure of RT_TOOLBAR resource
struct TOOLBARDATA {
	WORD wVersion;			// version # should be 1
	WORD wWidth;			// width of one bitmap
	WORD wHeight;			// height of one bitmap
	WORD wItemCount;		// number of items
	WORD items[1];			// array of command IDs, actual size is wItemCount
};

#define RT_TOOLBAR  MAKEINTRESOURCE(241)

bool CBitmapMenu::AddToolbar( int nTooobarID )
{
	LPTSTR lpResName = MAKEINTRESOURCE(nTooobarID);
	HRSRC hRsrc;
	TOOLBARDATA* ptbd;
	if( (hRsrc=FindResource(g_hInst, lpResName, RT_TOOLBAR)) == NULL ||
	    (ptbd=(TOOLBARDATA*)LoadResource(g_hInst, hRsrc)) == NULL )
		return false;
	CToolbarInfo toolbar;
	toolbar.hIL = ImageList_LoadImage( g_hInst, lpResName, 16, 0, RGB(192,192,192),	
		IMAGE_BITMAP, LR_DEFAULTCOLOR );
	HBITMAP hBmp = LoadBitmap( g_hInst, lpResName );
	for( int i=0,cnt=0; i<ptbd->wItemCount; i++ ) 
	{
		UINT nID = ptbd->items[i];
		if( !nID ) continue;
		toolbar.mCmd2Idx[ nID ] = cnt++;
	}
	m_toolbars.push_back( toolbar );
	return true;
}

void CBitmapMenu::OnInitMenuPopup( HMENU hMenu )
{
	if( m_toolbars.empty() ) return;

	MENUITEMINFO menuItemInfo = {0};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask  = MIIM_BITMAP;
	menuItemInfo.hbmpItem = HBMMENU_CALLBACK;
	for( int i=0; i<GetMenuItemCount(hMenu); i++ )
		SetMenuItemInfo(hMenu, i, TRUE, &menuItemInfo);

	MENUINFO menuInfo = {0};
	menuInfo.cbSize = sizeof(menuInfo);
	menuInfo.fMask = MIM_STYLE;
	GetMenuInfo(hMenu, &menuInfo);
	menuInfo.dwStyle |= MNS_NOCHECK;
	SetMenuInfo(hMenu, &menuInfo);
}

void CBitmapMenu::OnMeasureItem( MEASUREITEMSTRUCT* mi )
{
	if( mi->CtlType != ODT_MENU ) return;
//	mi->itemHeight = mi->itemHeight;
	mi->itemWidth = max(16, mi->itemWidth)+2+TEXT_SPACE;
}

void CBitmapMenu::OnDrawItem( DRAWITEMSTRUCT* di )
{
	if( di->CtlType != ODT_MENU ) return;

	// Get information about the menu item to be drawn.
	bool isDisabled = (di->itemState & ODS_DISABLED) != 0;
	bool isSelected = (di->itemState & ODS_SELECTED) != 0;
	bool isChecked = (di->itemState & ODS_CHECKED) != 0;
	HDC hDC = di->hDC;

	// Get the image that corresponds to this menu item.
	int imageID = -1;
	HIMAGELIST hIL;
	GetImage(di->itemID, hIL, imageID);
	if( imageID == -1 )
		if( m_pObj && m_OnGetImage )
			m_OnGetImage(m_pObj, di->itemID, hIL, imageID);
	bool bCheckedImage = false;
	if( imageID == -1 && isChecked ) {
		GetImage(ID_TAB_CHECKED, hIL, imageID);
		bCheckedImage = true;
	}
	if (imageID == -1) return;

	// Get the size and position of the button and bitmap.
	IMAGEINFO imageInfo;
	ImageList_GetImageInfo(hIL, imageID, &imageInfo);
	CSize bitmapSize = CRect(imageInfo.rcImage).Size();
	CSize buttonSize = CRect(di->rcItem).Size();
	buttonSize.cx -= 2 + TEXT_SPACE;
	CPoint bitmapPosition((buttonSize.cx - bitmapSize.cx) / 2 + 1,
						  (buttonSize.cy - bitmapSize.cy) / 2 + 1);

	if( isDisabled )
		DrawDisabled(hDC, hIL, imageID, bitmapPosition, bitmapSize);
	else
	if( isChecked ) {
		if( !bCheckedImage )
			DrawChecked(hDC, buttonSize);
		ImageList_Draw(hIL, imageID, hDC, bitmapPosition.x, bitmapPosition.y, ILD_TRANSPARENT);
	} else {
//		if( isSelected )
//			DrawButton(hDC, buttonSize);
		ImageList_Draw(hIL, imageID, hDC, bitmapPosition.x, bitmapPosition.y, ILD_TRANSPARENT);
	}
}

void CBitmapMenu::GetImage( UINT menuID, HIMAGELIST& hIL, int& imageID )
{
	for( auto toolbar: m_toolbars )
	{
		map<int,int>::iterator it = toolbar.mCmd2Idx.find(menuID);
		if( it==toolbar.mCmd2Idx.end() ) continue;
		hIL = toolbar.hIL;
		imageID = it->second;
		return;
	}
	imageID=-1;
}

void MoveTo(HDC hdc, int x, int y) { MoveToEx(hdc, x, y, NULL);  }

void CBitmapMenu::DrawButton( HDC dc, CSize &buttonSize )
{
	// Clear the area where the button will be drawn.
	HBRUSH brush = CreateSolidBrush(GetSysColor(COLOR_MENU));
	HBRUSH oldBrush = (HBRUSH)SelectObject(dc, brush);
	PatBlt(dc, 1, 1, buttonSize.cx, buttonSize.cy, PATCOPY);
	SelectObject(dc, oldBrush);
	DeleteObject(brush);

	HPEN lightPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNHIGHLIGHT));
	HPEN darkPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));

	HPEN oldPen = (HPEN)SelectObject(dc,lightPen);
	MoveTo(dc, 1, buttonSize.cy);
	LineTo(dc, 1, 1);
	LineTo(dc, buttonSize.cx, 1);
	SelectObject(dc,darkPen);
	LineTo(dc, buttonSize.cx, buttonSize.cy);
	LineTo(dc, 1, buttonSize.cy);
	SelectObject(dc,oldPen);

	DeleteObject(lightPen);
	DeleteObject(darkPen);
}

void CBitmapMenu::DrawChecked(HDC dc, CSize &buttonSize)
{
	HBRUSH brush = CreateSolidBrush(GetSysColor(COLOR_MENU));
	HBRUSH oldBrush = (HBRUSH)SelectObject(dc, brush);
	PatBlt(dc, 1, 1, buttonSize.cx, buttonSize.cy, PATCOPY);
	SelectObject(dc, oldBrush);
	DeleteObject(brush);

	HPEN lightPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNHIGHLIGHT));
	HPEN darkPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));

	HPEN oldPen = (HPEN)SelectObject(dc, darkPen);
	MoveTo(dc, 1, buttonSize.cy);
	LineTo(dc, 1, 1);
	LineTo(dc, buttonSize.cx, 1);
	SelectObject(dc, lightPen);
	LineTo(dc, buttonSize.cx, buttonSize.cy);
	LineTo(dc, 1, buttonSize.cy);
	SelectObject(dc, oldPen);

	DeleteObject(lightPen);
	DeleteObject(darkPen);
}

void CBitmapMenu::DrawDisabled( HDC dc, HIMAGELIST& hIL, int imageID, CPoint &position, CSize &size )
{
	// Create a color bitmap.
	HDC windowDC = GetWindowDC(0);
	HDC colorDC = CreateCompatibleDC(0);
	HBITMAP colorBmp = CreateCompatibleBitmap( windowDC, size.cx, size.cy );
	HBITMAP oldColorBmp = (HBITMAP)SelectObject( colorDC, colorBmp );

	// Create a monochrome bitmap.
	HDC monoDC = CreateCompatibleDC(0);
	HBITMAP monoBmp = CreateCompatibleBitmap( monoDC, size.cx, size.cy );
	HBITMAP oldMonoBmp = (HBITMAP)SelectObject( monoDC, monoBmp );

	// Copy the toolbar button to the color bitmap, make all transparent 
	// areas the same color as the button highlight color.
	IMAGELISTDRAWPARAMS ilDraw={0};
	ilDraw.cbSize = IMAGELISTDRAWPARAMS_V3_SIZE;
	ilDraw.himl = hIL;
	ilDraw.i = imageID;
	ilDraw.hdcDst = colorDC;
	ilDraw.x = 0;
	ilDraw.y = 0;
	ilDraw.cx = size.cx;
	ilDraw.cy = size.cy;
	ilDraw.xBitmap = 0;
	ilDraw.yBitmap = 0;
	ilDraw.fStyle = ILD_NORMAL;
	ilDraw.dwRop = SRCCOPY;
	ilDraw.rgbBk = GetSysColor(COLOR_BTNHIGHLIGHT);
	ImageList_DrawIndirect( &ilDraw );

	// Copy the color bitmap into the monochrome bitmap. Pixels that 
	// have the button highlight color are mapped to the background.
	SetBkColor( colorDC, GetSysColor(COLOR_BTNHIGHLIGHT) );
	BitBlt( monoDC, 0, 0, size.cx, size.cy, colorDC, 0, 0, SRCCOPY );

	// Draw the monochrome bitmap onto the menu.
	BitBlt( dc, position.x, position.y, size.cx, size.cy, monoDC, 0, 0, SRCCOPY );

	// Delete the color DC and bitmap.
	SelectObject( colorDC, oldColorBmp );
	DeleteDC(colorDC);
	DeleteObject(colorBmp);

	// Delete the monochrome DC and bitmap.
	SelectObject(monoDC, oldMonoBmp);
	DeleteDC(monoDC);
	DeleteObject(monoBmp);

	ReleaseDC(0, windowDC);
}