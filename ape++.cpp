#include "stdafx.h"
#include "resource.h"
#include "main.h"
#include "dlgFind.h"
#include <objbase.h> // CoInitialize
#include "options.h"
#include "dlgIntegr.h"
#include "lang.h"
#include "theme.h"
#include "ChordAccel.h" // multi-key ("chord") shortcuts

HINSTANCE g_hInst;
HWND g_hMainWnd;
HKEY g_hKey = HKEY_CURRENT_USER;
const char* g_szPath = R"(Software\vvvSoft\APE++\)";

#pragma comment(lib, "imm32.lib") // scintilla uses Internationalization for Windows Applications
#pragma comment(lib, "Msimg32.lib") // scintilla uses AlphaBlend()

ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEXW wcex{};
	wcex.cbSize = sizeof(wcex);
	wcex.style = 0;
	wcex.lpfnWndProc = CEditor::WndProcStatic;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(void*);
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APE));
	wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APE_SM));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = {};
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MENU);
	wcex.lpszClassName = APE_CLASS;
	return RegisterClassExW(&wcex);
}

bool apeInstanceCheck(HWND hWnd) {
	DWORD data = INSTANCE_CODE;
	COPYDATASTRUCT cds{.cbData = sizeof(data), .lpData = &data};
	DWORD_PTR res;
	bool b = SendMessageTimeout(hWnd, WM_COPYDATA, 0, (LPARAM) &cds, SMTO_ABORTIFHUNG, 1000, &res) != 0;
	return b && res;
}

BOOL CALLBACK enumProc(HWND hWnd, LPARAM lp) {
	BOOL ret = hWnd && IsWindow(hWnd) && getClassNameW(hWnd) == APE_CLASS && apeInstanceCheck(hWnd);
	if( ret )
		*(HWND*) lp = hWnd;
	return !ret;
}

HWND apeFindWindow() {
	HWND h = nullptr;
	EnumWindows(enumProc, (LPARAM) &h);
	return h;
}

bool apeInstanceCopyData(HWND hWnd, const string &s) {
	COPYDATASTRUCT cds{.cbData = (DWORD) s.size(), .lpData = (void *) s.c_str()};
	return SendMessage(hWnd, WM_COPYDATA, 0, (LPARAM) &cds) != 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int) {
#ifdef _DEBUG
	void testShared(); testShared();
	void testLib(); testLib();
#endif

	vector<string> vCmdLine;
	if( lpCmdLine && lpCmdLine[0] ) {
//		MessageBoxW(nullptr, lpCmdLine, nullptr, MB_OK);
		string sCmdLine = w2utf(lpCmdLine);
		splitQ(sCmdLine, vCmdLine, ' ');

		if( !vCmdLine.empty() && vCmdLine[0] == _INTEGR ) {
			g_hInst = hInstance;
			HWND hParent = nullptr;
			if( vCmdLine.size() > 1 ) {
				hParent = (HWND) (UINT_PTR) stoull(vCmdLine[1]);
				if( !IsWindow(hParent) )
					hParent = nullptr;
			}
			InitCommonControls();
			g_lang.init(ID_LANG_FIRST, ID_LANG_LAST);
			g_lang.setLang(LoadIntVal("", "lang", -1));
			CIntegrationDlg().doModal(hParent);
			return 0;
		}

		if( vCmdLine.size() >= 2 && vCmdLine[0] == "-d" && containsIC(vCmdLine[1], "notepad") ) {
			if( vCmdLine.size() == 2 && isNotepadReplacement() ) {
				setNotepadReplacement(false, false);
				if( CreateProcess(vCmdLine[1]) )
					Sleep(1000);
				setNotepadReplacement(true, false);
				return 0;
			} else {
				vCmdLine.erase(vCmdLine.begin(), vCmdLine.begin() + 2); // -d notepad.exe
				string str = combine(vCmdLine, ' ');
				vCmdLine.clear();
				vCmdLine.push_back(str);
			}
		}
		for( string& str: vCmdLine )
			EnsureFilePath(str);
	}

//	if another instance is already running, pass to it our files (if any) and exit.
	bool bNewInstance = !vCmdLine.empty() && vCmdLine[0] == _NEW;
	if( g_options.bOneInstanse && !bNewInstance ) {
		HWND hAPE = apeFindWindow();
		if( hAPE ) {
			string sFiles = combine(vCmdLine, 0x9);
			AllowSetForegroundWindow(ASFW_ANY); // allow that instance to bring itself to front
			if( sFiles.empty() || apeInstanceCopyData(hAPE, sFiles) ) {
				ShowWindow( hAPE, IsIconic(hAPE) ? SW_RESTORE : SW_SHOW );
				::SetForegroundWindow(hAPE);
				return 0;
			}
		}
	}

	Scintilla_RegisterClasses(hInstance);
	InitCommonControls();
	CoInitialize(nullptr);
	MyRegisterClass(hInstance);
	g_theme.init(g_options.iTheme); // dark popup menus must be set before the window is created

	HWND hWnd = CreateWindowW(APE_CLASS, APE_TITLE,
	                          WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
	                          CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	                          nullptr, nullptr, hInstance, nullptr);
	if( !hWnd )
		return 0;

	g_hInst = hInstance;
	g_hMainWnd = hWnd;

	CEditor editor;
	editor.onCreate(hWnd, bNewInstance);

	if( vCmdLine.size() == 4 && vCmdLine[0] == _NEW && !vCmdLine[1].empty() &&
			startsWith(vCmdLine[2], _TOP) && startsWith(vCmdLine[3], _POS) ) {
		int top = stoi(vCmdLine[2].substr(strlen(_TOP)));
		int pos = stoi(vCmdLine[3].substr(strlen(_POS)));
		editor.FileOpen(vCmdLine[1], false, -1, top, pos);
	} else {
		for( const auto& str: vCmdLine )
			editor.FileOpen(str, false);
	}
	editor.onTabChanged();

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ACCEL));
	MSG msg;
	while( GetMessage(&msg, nullptr, 0, 0) ) {
		if( !editor.m_findDlg.isDlgMsg(msg) )
			if( !TranslateChordAccel(msg) )
				if( !TranslateAccelerator(g_hMainWnd, hAccelTable, &msg) ) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
	}

	return (int) msg.wParam;
}