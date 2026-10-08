#include "BitmapMenu.h"
#include "resource.h" // for ID_TAB_CHECKED
#include "theme.h"
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

	// HBMMENU_CALLBACK turns off the visual styles of the popup, so it is always light;
	// in dark mode the items get 32-bit alpha bitmaps that the themed menu draws itself
	bool bDark = g_theme.isDark();
	MENUITEMINFO menuItemInfo = {0};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	for( int i=0; i<GetMenuItemCount(hMenu); i++ ) {
		menuItemInfo.fMask = MIIM_BITMAP;
		menuItemInfo.hbmpItem = HBMMENU_CALLBACK;
		if( bDark ) {
			menuItemInfo.hbmpItem = nullptr;
			menuItemInfo.fMask = MIIM_ID | MIIM_STATE | MIIM_SUBMENU;
			if( GetMenuItemInfo(hMenu, i, TRUE, &menuItemInfo) && !menuItemInfo.hSubMenu ) {
				int imageID = -1;
				HIMAGELIST hIL;
				GetImage(menuItemInfo.wID, hIL, imageID);
				if( imageID == -1 && m_pObj && m_OnGetImage )
					m_OnGetImage(m_pObj, menuItemInfo.wID, hIL, imageID);
				if( imageID != -1 ) // checked items without image get the check mark of the theme
					menuItemInfo.hbmpItem = GetBitmap(hIL, imageID, (menuItemInfo.fState & MFS_DISABLED) != 0);
			}
			menuItemInfo.fMask = MIIM_BITMAP;
		}
		SetMenuItemInfo(hMenu, i, TRUE, &menuItemInfo);
	}

	MENUINFO menuInfo = {0};
	menuInfo.cbSize = sizeof(menuInfo);
	menuInfo.fMask = MIM_STYLE;
	GetMenuInfo(hMenu, &menuInfo);
	menuInfo.dwStyle &= ~(MNS_NOCHECK | MNS_CHECKORBMP);
	menuInfo.dwStyle |= bDark ? MNS_CHECKORBMP : MNS_NOCHECK;
	SetMenuInfo(hMenu, &menuInfo);
}

// premultiplied 32-bit bitmap: the image is drawn on black and on white, the difference gives the alpha
HBITMAP CBitmapMenu::GetBitmap( HIMAGELIST hIL, int imageID, bool bDisabled )
{
	auto key = std::make_tuple(hIL, imageID, bDisabled);
	auto it = m_bitmaps.find(key);
	if( it != m_bitmaps.end() )
		return it->second;

	int cx = 16, cy = 16;
	ImageList_GetIconSize(hIL, &cx, &cy);
	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = cx;
	bmi.bmiHeader.biHeight = -cy; // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	HDC hDC = CreateCompatibleDC(nullptr);
	BYTE* pBlack = nullptr;
	BYTE* pWhite = nullptr;
	HBITMAP hBlack = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, (void**)&pBlack, nullptr, 0);
	HBITMAP hWhite = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, (void**)&pWhite, nullptr, 0);
	if( !hBlack || !hWhite ) {
		if( hBlack ) DeleteObject(hBlack);
		if( hWhite ) DeleteObject(hWhite);
		DeleteDC(hDC);
		return m_bitmaps[key] = nullptr;
	}
	RECT rc = {0, 0, cx, cy};
	HGDIOBJ hOld = SelectObject(hDC, hBlack);
	FillRect(hDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
	ImageList_Draw(hIL, imageID, hDC, 0, 0, ILD_TRANSPARENT);
	SelectObject(hDC, hWhite);
	FillRect(hDC, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
	ImageList_Draw(hIL, imageID, hDC, 0, 0, ILD_TRANSPARENT);
	SelectObject(hDC, hOld);
	DeleteDC(hDC);
	GdiFlush();

	for( int i=0; i<cx*cy; i++ ) {
		BYTE* b = pBlack + i*4;
		const BYTE* w = pWhite + i*4;
		int a = 255 - (w[1] - b[1]); // green: the most precise channel
		if( a < 0 ) a = 0;
		if( a > 255 ) a = 255;
		if( bDisabled ) { // grey and semi-transparent
			int lum = (b[2]*30 + b[1]*59 + b[0]*11) / 100;
			a = a * 45 / 100;
			lum = lum * 45 / 100;
			b[0] = b[1] = b[2] = (BYTE)lum;
		}
		for( int c=0; c<3; c++ )
			if( b[c] > a ) b[c] = (BYTE)a;
		b[3] = (BYTE)a; // the black background makes the colors premultiplied
	}
	DeleteObject(hWhite);
	return m_bitmaps[key] = hBlack;
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
	HBRUSH brush = CreateSolidBrush(g_theme.isDark() ? CTheme::clrPressed : GetSysColor(COLOR_MENU));
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
	// white pixels get the bk color of dc, black pixels the text color
	COLORREF clrOldBk = 0, clrOldText = 0;
	if( g_theme.isDark() ) {
		clrOldBk = SetBkColor( dc, CTheme::clrMenu );
		clrOldText = SetTextColor( dc, CTheme::clrTextDisabled );
	}
	BitBlt( dc, position.x, position.y, size.cx, size.cy, monoDC, 0, 0, SRCCOPY );
	if( g_theme.isDark() ) {
		SetBkColor( dc, clrOldBk );
		SetTextColor( dc, clrOldText );
	}

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