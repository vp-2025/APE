#include "stdafx.h"
#include "resource.h"
#include "main.h"
#include "dlgFind.h"
#include <objbase.h> // CoInitialize
#include "options.h"
#include "dlgIntegr.h"

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

bool InstanceCheck(HWND hWnd) {
	DWORD data = INSTANCE_CODE;
	COPYDATASTRUCT cds{};
	cds.cbData = sizeof(data);
	cds.lpData = &data;
	DWORD res;
	bool b = SendMessageTimeout(hWnd, WM_COPYDATA, 0, (LPARAM) &cds, SMTO_ABORTIFHUNG, 1000, &res) != 0;
	return b && res;
}

bool InstanceCopyData(HWND hWnd, string s) {
	COPYDATASTRUCT cds{};
	cds.cbData = s.size();
	cds.lpData = (void*) s.c_str();
	return SendMessage(hWnd, WM_COPYDATA, 0, (LPARAM) &cds) != 0;
}

BOOL CALLBACK enumProc(HWND hWnd, LPARAM lp) {
	BOOL ret = hWnd && IsWindow(hWnd) && getClassNameW(hWnd) == APE_CLASS && InstanceCheck(hWnd);
	if( ret )
		*((HWND*) lp) = hWnd;
	return !ret;
}

HWND apeFindWindow() {
	HWND h = nullptr;
	EnumWindows(enumProc, (LPARAM) &h);
	return h;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
#ifdef _DEBUG
	void testShared(); testShared();
	void testLib(); testLib();
#endif

	vector<string> vCmdLine;
	if( lpCmdLine && lpCmdLine[0] ) {
//		MessageBoxW(nullptr, lpCmdLine, nullptr, MB_OK);
		string sCmdLine = w2utf(lpCmdLine);
		splitQ(sCmdLine, vCmdLine, ' ');
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
		CEditor::bNewInstance = !vCmdLine.empty() && vCmdLine[0] == _NEW;
		if( g_options.bOneInstanse && !CEditor::bNewInstance ) {
			HWND hAPE = apeFindWindow();
			if( hAPE && InstanceCopyData(hAPE, combine(vCmdLine, 0x9)) ) {
				if( IsIconic(hAPE) )
					ShowWindow(hAPE, SW_RESTORE);
				::SetForegroundWindow(hAPE);
				return 0;
			}
		}
	}

	Scintilla_RegisterClasses(hInstance);
	InitCommonControls();
	CoInitialize(nullptr);
	MyRegisterClass(hInstance);

	HWND hWnd = CreateWindowW(APE_CLASS, APE_TITLE,
	                          WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
	                          CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	                          nullptr, nullptr, hInstance, nullptr);
	if( !hWnd )
		return 0;

	g_hInst = hInstance;
	g_hMainWnd = hWnd;

	CEditor editor;
	editor.onCreate(hWnd);

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
			if( !TranslateAccelerator(g_hMainWnd, hAccelTable, &msg) ) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
	}

	return (int) msg.wParam;
}