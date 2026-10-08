#include "stdafx.h"
#include "theme.h"
#include <string>

#pragma comment(lib, "comctl32.lib") // SetWindowSubclass

CTheme g_theme;

namespace {

// uxtheme ordinals used by Explorer / Notepad / Notepad++ for the dark mode; they exist since Windows 10 1809
enum PreferredAppMode { AppModeDefault, AppModeAllowDark, AppModeForceDark, AppModeForceLight };

using FnSetPreferredAppMode = int (WINAPI*)(int);                   // #135, 1903+
using FnAllowDarkModeForApp = BOOL (WINAPI*)(BOOL);                 // #135, 1809
using FnAllowDarkModeForWindow = BOOL (WINAPI*)(HWND, BOOL);        // #133
using FnFlushMenuThemes = void (WINAPI*)();                         // #136
using FnRefreshImmersiveColorPolicyState = void (WINAPI*)();        // #104
using FnSetWindowTheme = HRESULT (WINAPI*)(HWND, LPCWSTR, LPCWSTR);
using FnDwmSetWindowAttribute = HRESULT (WINAPI*)(HWND, DWORD, LPCVOID, DWORD);

#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32 0x00000800
#endif

constexpr DWORD BUILD_1809 = 17763;
constexpr DWORD BUILD_1903 = 18362;

struct CDarkApi {
	DWORD build{};
	FnSetWindowTheme setWindowTheme{};
	FnSetPreferredAppMode setPreferredAppMode{};
	FnAllowDarkModeForApp allowDarkModeForApp{};
	FnAllowDarkModeForWindow allowDarkModeForWindow{};
	FnFlushMenuThemes flushMenuThemes{};
	FnRefreshImmersiveColorPolicyState refreshImmersiveColorPolicyState{};
	FnDwmSetWindowAttribute dwmSetWindowAttribute{};

	CDarkApi() {
		build = getBuildNumber();
		if( HMODULE hUx = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32) ) {
			setWindowTheme = (FnSetWindowTheme)GetProcAddress(hUx, "SetWindowTheme");
			// on older builds the same ordinals are other functions
			if( build >= BUILD_1809 ) {
				if( build >= BUILD_1903 )
					setPreferredAppMode = (FnSetPreferredAppMode)GetProcAddress(hUx, MAKEINTRESOURCEA(135));
				else
					allowDarkModeForApp = (FnAllowDarkModeForApp)GetProcAddress(hUx, MAKEINTRESOURCEA(135));

//				allowDarkModeForWindow = (FnAllowDarkModeForWindow)GetProcAddress(hUx, MAKEINTRESOURCEA(133));
//				flushMenuThemes = (FnFlushMenuThemes)GetProcAddress(hUx, MAKEINTRESOURCEA(136));
//				refreshImmersiveColorPolicyState = (FnRefreshImmersiveColorPolicyState)GetProcAddress(hUx, MAKEINTRESOURCEA(104));
			}
		}
		if( HMODULE hDwm = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32) )
			dwmSetWindowAttribute = (FnDwmSetWindowAttribute)GetProcAddress(hDwm, "DwmSetWindowAttribute");
	}

	static DWORD getBuildNumber() {
		using FnRtlGetVersion = LONG (WINAPI*)(OSVERSIONINFOW*);
		auto fn = (FnRtlGetVersion)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
		OSVERSIONINFOW vi{.dwOSVersionInfoSize = sizeof(vi)};
		return fn && fn(&vi) == 0 ? vi.dwBuildNumber : 0;
	}

	bool isSupported() const { return build >= BUILD_1809; }

	void setAppMode(bool bDark) const {
		if( setPreferredAppMode )
			setPreferredAppMode(bDark ? AppModeForceDark : AppModeForceLight);
		else if( allowDarkModeForApp )
			allowDarkModeForApp(bDark);
		if( refreshImmersiveColorPolicyState )
			refreshImmersiveColorPolicyState();
		if( flushMenuThemes )
			flushMenuThemes();
	}
};

CDarkApi& api() {
	static CDarkApi a;
	return a;
}

bool isHighContrast() {
	HIGHCONTRASTW hc{};
	hc.cbSize = sizeof(hc);
	return SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0) && (hc.dwFlags & HCF_HIGHCONTRASTON);
}

HFONT systemFont(bool bMenu) {
	static HFONT hMenu = nullptr, hStatus = nullptr;
	HFONT& h = bMenu ? hMenu : hStatus;
	if( !h ) {
		NONCLIENTMETRICSW ncm{};
		ncm.cbSize = sizeof(ncm);
		if( SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0) )
			h = CreateFontIndirectW(bMenu ? &ncm.lfMenuFont : &ncm.lfStatusFont);
		if( !h )
			h = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	}
	return h;
}

/////////////////////////////////////////////////////////////////////
// HSL, to convert the editor colors

struct CHsl { double h, s, l; };

double dmax( double a, double b ) { return a > b ? a : b; }
double dmin( double a, double b ) { return a < b ? a : b; }

CHsl toHsl(COLORREF clr) {
	double r = GetRValue(clr) / 255.0;
	double g = GetGValue(clr) / 255.0;
	double b = GetBValue(clr) / 255.0;
	double mx = dmax( dmax( r, g ), b );
	double mn = dmin( dmin( r, g ), b );
	double d = mx - mn;
	
	CHsl x{0, 0, (mx + mn) / 2};
	if( d > 0 ) {
		x.s = x.l > 0.5 ? d / (2 - mx - mn) : d / (mx + mn);
		if( mx == r ) x.h = (g - b) / d + (g < b ? 6 : 0);
		else if( mx == g ) x.h = (b - r) / d + 2;
		else x.h = (r - g) / d + 4;
		x.h /= 6;
	}
	return x;
}

double hue2rgb(double p, double q, double t) {
	if( t < 0 ) t += 1;
	if( t > 1 ) t -= 1;
	if( t < 1 / 6.0 ) return p + (q - p) * 6 * t;
	if( t < 1 / 2.0 ) return q;
	if( t < 2 / 3.0 ) return p + (q - p) * (2 / 3.0 - t) * 6;
	return p;
}

COLORREF fromHsl(const CHsl& x) {
	double r = x.l, g = x.l, b = x.l;
	if( x.s > 0 ) {
		double q = x.l < 0.5 ? x.l * (1 + x.s) : x.l + x.s - x.l * x.s;
		double p = 2 * x.l - q;
		r = hue2rgb(p, q, x.h + 1 / 3.0);
		g = hue2rgb(p, q, x.h);
		b = hue2rgb(p, q, x.h - 1 / 3.0);
	}
	return RGB((int)(r * 255 + 0.5), (int)(g * 255 + 0.5), (int)(b * 255 + 0.5));
}

/////////////////////////////////////////////////////////////////////
// menu bar: undocumented messages, the same as Notepad++ uses

constexpr UINT WM_UAHDRAWMENU = 0x0091;
constexpr UINT WM_UAHDRAWMENUITEM = 0x0092;

struct UAHMENU {
	HMENU hmenu;
	HDC hdc;
	DWORD dwFlags;
};

union UAHMENUITEMMETRICS {
	struct { DWORD cx, cy; } rgsizeBar[2];
	struct { DWORD cx, cy; } rgsizePopup[4];
};

struct UAHMENUPOPUPMETRICS {
	DWORD rgcx[4];
	DWORD fUpdateMaxWidths : 2;
};

struct UAHMENUITEM {
	int iPosition;
	UAHMENUITEMMETRICS umim;
	UAHMENUPOPUPMETRICS umpm;
};

struct UAHDRAWMENUITEM {
	DRAWITEMSTRUCT dis;
	UAHMENU um;
	UAHMENUITEM umi;
};

// the system draws a light line between the menu bar and the client area
void drawMenuBarBottomLine(HWND hWnd, HBRUSH hbr) {
	MENUBARINFO mbi{};
	mbi.cbSize = sizeof(mbi);
	if( !GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi) )
		return;
	RECT rcClient, rcWindow;
	GetClientRect(hWnd, &rcClient);
	MapWindowPoints(hWnd, nullptr, (POINT*)&rcClient, 2);
	GetWindowRect(hWnd, &rcWindow);
	OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);
	RECT rcLine = rcClient;
	rcLine.bottom = rcLine.top;
	rcLine.top--;
	HDC hdc = GetWindowDC(hWnd);
	FillRect(hdc, &rcLine, hbr);
	ReleaseDC(hWnd, hdc);
}

/////////////////////////////////////////////////////////////////////
// rebar + toolbar

#ifndef TBCDRF_NOBACKGROUND
#define TBCDRF_NOBACKGROUND 0x00400000
#endif

LRESULT onToolbarCustomDraw(NMTBCUSTOMDRAW* p) {
	switch( p->nmcd.dwDrawStage ) {
		case CDDS_PREPAINT: {
			RECT rc;
			GetClientRect(p->nmcd.hdr.hwndFrom, &rc);
			FillRect(p->nmcd.hdc, &rc, g_theme.brBack());
			return CDRF_NOTIFYITEMDRAW;
		}
		case CDDS_ITEMPREPAINT: {
			UINT st = p->nmcd.uItemState;
			if( st & (CDIS_SELECTED | CDIS_CHECKED) )
				FillRect(p->nmcd.hdc, &p->nmcd.rc, g_theme.brPressed());
			else if( st & CDIS_HOT )
				FillRect(p->nmcd.hdc, &p->nmcd.rc, g_theme.brHot());
			return TBCDRF_NOEDGES | TBCDRF_NOBACKGROUND | TBCDRF_NOOFFSET;
		}
	}
	return CDRF_DODEFAULT;
}

void fillClient(HWND hWnd, HDC hdc) {
	RECT rc;
	GetClientRect(hWnd, &rc);
	FillRect(hdc, &rc, g_theme.brBack());
}

LRESULT CALLBACK ReBarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwToolbar) {
	if( g_theme.isDark() ) {
		switch( msg ) {
			case WM_ERASEBKGND: // also the background of the transparent toolbar
				fillClient(hWnd, (HDC)wParam);
				return TRUE;
			case WM_PRINTCLIENT:
				fillClient(hWnd, (HDC)wParam);
				return 0;
			case WM_PAINT: { // skip the light band borders and grippers
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);
				FillRect(hdc, &ps.rcPaint, g_theme.brBack());
				EndPaint(hWnd, &ps);
				return 0;
			}
			case WM_NOTIFY: {
				auto* pNM = (NMHDR*)lParam;
				if( pNM->code == NM_CUSTOMDRAW && pNM->hwndFrom == (HWND)dwToolbar )
					return onToolbarCustomDraw((NMTBCUSTOMDRAW*)lParam);
				break;
			}
		}
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

// the toolbar is the parent of the functions combobox
LRESULT CALLBACK ToolbarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
	switch( msg ) {
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
			if( HBRUSH hbr = g_theme.onCtlColor((HDC)wParam, true) )
				return (LRESULT)hbr;
			break;
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////
// statusbar: the text color of the common control can't be changed, so it is painted here

void paintStatusbar(HWND hWnd) {
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	RECT rc;
	GetClientRect(hWnd, &rc);
	FillRect(hdc, &rc, g_theme.brBack());
	RECT rcTop = rc;
	rcTop.bottom = rcTop.top + 1;
	FillRect(hdc, &rcTop, g_theme.brBorder());

	HFONT hFont = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
	HGDIOBJ hOldFont = SelectObject(hdc, hFont ? hFont : systemFont(false));
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, CTheme::clrText);

	int cnt = (int)SendMessageW(hWnd, SB_GETPARTS, 0, 0);
	for( int i = 0; i < cnt; i++ ) {
		RECT r;
		if( !SendMessageW(hWnd, SB_GETRECT, i, (LPARAM)&r) )
			continue;
		int len = LOWORD(SendMessageW(hWnd, SB_GETTEXTLENGTHW, i, 0));
		std::wstring s(len + 1, L'\0'); // +1 for ending zero
		if( len )
			SendMessageW(hWnd, SB_GETTEXTW, i, (LPARAM)&s[0]);
		RECT rText = r;
		rText.left += 4;
		rText.right -= 4;
		DrawTextW(hdc, s.c_str(), len, &rText, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
		if( i < cnt - 1 ) {
			RECT rSep = {r.right - 1, r.top + 3, r.right, r.bottom - 3};
			FillRect(hdc, &rSep, g_theme.brBorder());
		}
	}
	SelectObject(hdc, hOldFont);
	EndPaint(hWnd, &ps);
}

LRESULT CALLBACK StatusbarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
	if( g_theme.isDark() ) {
		switch( msg ) {
			case WM_ERASEBKGND:
				return TRUE;
			case WM_PAINT:
				paintStatusbar(hWnd);
				return 0;
		}
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

} // namespace

/////////////////////////////////////////////////////////////////////

bool CTheme::isSystemDark() {
	if( !api().isSupported() || isHighContrast() ) // high contrast has own colors
		return false;
	DWORD value = 1, size = sizeof(value);
	HKEY hKey;
	if( RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0,
	                  KEY_READ, &hKey) == ERROR_SUCCESS ) {
		if( RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, (BYTE*)&value, &size) != ERROR_SUCCESS )
			value = 1;
		RegCloseKey(hKey);
	}
	return value == 0;
}

bool CTheme::isThemeChangeMsg(UINT msg, LPARAM lParam) {
	return msg == WM_SETTINGCHANGE && lParam && lstrcmpiW((LPCWSTR)lParam, L"ImmersiveColorSet") == 0;
}

static bool isDarkForMode(int mode) {
	switch( mode ) {
		case themeLight: return false;
		case themeDark: return api().isSupported() && !isHighContrast(); // high contrast has own colors
		default: return CTheme::isSystemDark();
	}
}

void CTheme::init(int mode) {
	m_mode = mode;
	m_bDark = isDarkForMode(m_mode);
	api().setAppMode(m_bDark);
}

bool CTheme::setMode(int mode) {
	m_mode = mode;
	return update();
}

bool CTheme::update() {
	bool bDark = isDarkForMode(m_mode);
	if( bDark == m_bDark )
		return false;
	m_bDark = bDark;
	api().setAppMode(m_bDark);
	return true;
}

static HBRUSH& lazyBrush(HBRUSH& hbr, COLORREF clr) {
	if( !hbr )
		hbr = CreateSolidBrush(clr);
	return hbr;
}

HBRUSH CTheme::brBack() { return lazyBrush(m_hbrBack, clrBack); }
HBRUSH CTheme::brCtrl() { return lazyBrush(m_hbrCtrl, clrCtrl); }
HBRUSH CTheme::brHot() { return lazyBrush(m_hbrHot, clrHot); }
HBRUSH CTheme::brPressed() { return lazyBrush(m_hbrPressed, clrPressed); }
HBRUSH CTheme::brBorder() { return lazyBrush(m_hbrBorder, clrBorder); }

void CTheme::applyTitleBar(HWND hWnd, bool bRepaint) const {
	const CDarkApi& a = api();
	if( !a.isSupported() || !hWnd )
		return;
	if( a.allowDarkModeForWindow )
		a.allowDarkModeForWindow(hWnd, m_bDark);
	if( a.dwmSetWindowAttribute ) {
		BOOL b = m_bDark;
		// DWMWA_USE_IMMERSIVE_DARK_MODE is 20 since Windows 10 20H1, 19 before
		if( FAILED(a.dwmSetWindowAttribute(hWnd, 20, &b, sizeof(b))) )
			a.dwmSetWindowAttribute(hWnd, 19, &b, sizeof(b));
	}
	if( bRepaint && IsWindowVisible(hWnd) ) {
		// Windows 10 repaints the caption only on activation change
		SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		if( GetActiveWindow() == hWnd ) {
			DefWindowProcW(hWnd, WM_NCACTIVATE, FALSE, 0);
			DefWindowProcW(hWnd, WM_NCACTIVATE, TRUE, 0);
		}
	}
}

void CTheme::applyCtrl(HWND hWnd) const {
	const CDarkApi& a = api();
	if( !hWnd || !a.isSupported() )
		return;
	if( a.allowDarkModeForWindow )
		a.allowDarkModeForWindow(hWnd, m_bDark);
	if( !a.setWindowTheme )
		return;
	wchar_t szClass[64]{};
	GetClassNameW(hWnd, szClass, 64);
	bool bCombo = lstrcmpiW(szClass, WC_COMBOBOXW) == 0;
	if( m_bDark )
		a.setWindowTheme(hWnd, bCombo ? L"DarkMode_CFD" : L"DarkMode_Explorer", nullptr);
	else
		a.setWindowTheme(hWnd, nullptr, nullptr);
	if( bCombo ) {
		COMBOBOXINFO ci{};
		ci.cbSize = sizeof(ci);
		if( GetComboBoxInfo(hWnd, &ci) && ci.hwndList )
			applyCtrl(ci.hwndList);
	}
}

void CTheme::applySci(HWND hSci) const {
	applyCtrl(hSci);
	LONG_PTR ex = GetWindowLongPtr(hSci, GWL_EXSTYLE);
	LONG_PTR exNew = m_bDark ? (ex & ~WS_EX_CLIENTEDGE) : (ex | WS_EX_CLIENTEDGE);
	if( ex != exNew ) {
		SetWindowLongPtr(hSci, GWL_EXSTYLE, exNew);
		SetWindowPos(hSci, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}
}

COLORREF CTheme::editorFore(COLORREF clr) const {
	if( !m_bDark )
		return clr;
	CHsl x = toHsl(clr);
	x.l = 0.88 - 0.76 * x.l;      // black -> light grey, white -> background
	if( x.s > 0.25 && x.l < 0.65 ) // saturated colors (blue keywords, navy numbers) must stay readable
		x.l = 0.65;
	x.s *= 0.85;
	return fromHsl(x);
}

COLORREF CTheme::editorBack(COLORREF clr) const {
	if( !m_bDark )
		return clr;
	CHsl x = toHsl(clr);
	x.l = 0.118 + (1 - x.l) * 0.3; // white -> clrEditorBack, light tints -> dark tints
	x.s *= 0.6;
	return fromHsl(x);
}

bool CTheme::onMenuBarMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& lr) {
	if( !m_bDark )
		return false;
	switch( msg ) {
		case WM_UAHDRAWMENU: {
			auto* pUDM = (UAHMENU*)lParam;
			MENUBARINFO mbi{};
			mbi.cbSize = sizeof(mbi);
			if( !GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi) )
				return false;
			RECT rcWindow;
			GetWindowRect(hWnd, &rcWindow);
			RECT rc = mbi.rcBar;
			OffsetRect(&rc, -rcWindow.left, -rcWindow.top);
			FillRect(pUDM->hdc, &rc, brBack());
			lr = TRUE;
			return true;
		}
		case WM_UAHDRAWMENUITEM: {
			auto* pUDMI = (UAHDRAWMENUITEM*)lParam;
			wchar_t szText[256]{};
			MENUITEMINFOW mii{};
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = szText;
			mii.cch = 255;
			GetMenuItemInfoW(pUDMI->um.hmenu, pUDMI->umi.iPosition, TRUE, &mii);

			UINT state = pUDMI->dis.itemState;
			HDC hdc = pUDMI->um.hdc;
			HBRUSH hbr = brBack();
			if( state & ODS_SELECTED )
				hbr = brPressed();
			else if( state & ODS_HOTLIGHT )
				hbr = brHot();
			FillRect(hdc, &pUDMI->dis.rcItem, hbr);

			UINT dt = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
			if( state & ODS_NOACCEL )
				dt |= DT_HIDEPREFIX;
			bool bDisabled = (state & (ODS_INACTIVE | ODS_GRAYED | ODS_DISABLED)) != 0;
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, bDisabled ? clrTextDisabled : clrText);
			HGDIOBJ hOldFont = SelectObject(hdc, systemFont(true));
			DrawTextW(hdc, szText, -1, &pUDMI->dis.rcItem, dt);
			SelectObject(hdc, hOldFont);
			lr = TRUE;
			return true;
		}
		case WM_NCPAINT:
		case WM_NCACTIVATE:
			lr = DefWindowProcW(hWnd, msg, wParam, lParam);
			drawMenuBarBottomLine(hWnd, brBack());
			return true;
	}
	return false;
}

HBRUSH CTheme::onCtlColor(HDC hDC, bool bCtrl) {
	if( !m_bDark )
		return nullptr;
	SetTextColor(hDC, clrText);
	SetBkColor(hDC, bCtrl ? clrCtrl : clrBack);
	return bCtrl ? brCtrl() : brBack();
}

void CTheme::subclassReBar(HWND hReBar, HWND hToolbar) {
	SetWindowSubclass(hReBar, ReBarProc, 1, (DWORD_PTR)hToolbar);
	SetWindowSubclass(hToolbar, ToolbarProc, 1, 0);
}

void CTheme::subclassStatusbar(HWND hStatusbar) {
	SetWindowSubclass(hStatusbar, StatusbarProc, 1, 0);
}
