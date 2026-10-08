#pragma once

#include <windows.h>
#include <commctrl.h>

// Dark theme: follows the Windows "app mode" (Settings > Personalization > Colors).
// The editor colors from the styles tables (and user changes in the Colors dialog) are kept as they are,
// in dark mode they are converted on the fly, so the light theme and the saved styles are not touched.
enum CThemeMode { themeSystem, themeLight, themeDark }; // saved in options

class CTheme {
	bool m_bDark{};
	int m_mode{themeSystem};
	HBRUSH m_hbrBack{}, m_hbrCtrl{}, m_hbrHot{}, m_hbrPressed{}, m_hbrBorder{};
public:
	static constexpr COLORREF clrBack = RGB(0x20, 0x20, 0x20);         // bars, tab strip
	static constexpr COLORREF clrCtrl = RGB(0x2D, 0x2D, 0x2D);         // selected tab, combo
	static constexpr COLORREF clrHot = RGB(0x3D, 0x3D, 0x3D);          // hot item
	static constexpr COLORREF clrPressed = RGB(0x50, 0x50, 0x50);      // pressed / checked item
	static constexpr COLORREF clrBorder = RGB(0x45, 0x45, 0x45);
	static constexpr COLORREF clrText = RGB(0xE0, 0xE0, 0xE0);
	static constexpr COLORREF clrTextDisabled = RGB(0x80, 0x80, 0x80);
	static constexpr COLORREF clrEditorBack = RGB(0x1E, 0x1E, 0x1E);
	static constexpr COLORREF clrMargin = RGB(0x25, 0x25, 0x25);       // line numbers & fold margin
	static constexpr COLORREF clrMenu = RGB(0x2B, 0x2B, 0x2B);         // popup menu of Windows dark mode

	bool isDark() const { return m_bDark; }
	static bool isSystemDark();
	static bool isThemeChangeMsg(UINT msg, LPARAM lParam); // WM_SETTINGCHANGE "ImmersiveColorSet"

	int getMode() const { return m_mode; }
	void init(int mode);    // call before the main window is created
	bool setMode(int mode); // true if dark/light is changed
	bool update();          // re-reads the Windows setting; true if dark/light is changed

	HBRUSH brBack();
	HBRUSH brCtrl();
	HBRUSH brHot();
	HBRUSH brPressed();
	HBRUSH brBorder();

	void applyTitleBar(HWND hWnd, bool bRepaint = false) const;
	void applyCtrl(HWND hWnd) const; // scrollbars, tooltips, combobox
	void applySci(HWND hSci) const;  // + no 3D border in dark mode

	// editor colors
	COLORREF editorFore(COLORREF clr) const;
	COLORREF editorBack(COLORREF clr) const;

	// main window: dark menu bar; returns true if msg is handled
	bool onMenuBarMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& lr);
	// WM_CTLCOLOR*: returns brush or nullptr in light mode
	HBRUSH onCtlColor(HDC hDC, bool bCtrl = false);

	void subclassReBar(HWND hReBar, HWND hToolbar);
	void subclassStatusbar(HWND hStatusbar);
};

extern CTheme g_theme;
