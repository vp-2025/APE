#pragma once

#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <windowsx.h>

#include <vector>
#include <map>
#include <set>
#include <string>
#include <fstream>
#include <sstream>

#include <cassert>
#define ASSERT assert

extern HINSTANCE g_hInst;
extern HWND g_hMainWnd;

constexpr auto APE_CLASS = L"APEpp";
constexpr auto APE_TITLE = L"APE++";
constexpr auto _NEW = "-new";
constexpr auto _TOP = "-top=";
constexpr auto _POS = "-pos=";

constexpr auto _INTEGR = "-integration";

constexpr auto INSTANCE_CODE = 0xDEADBEEF;