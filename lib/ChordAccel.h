#pragma once

#include <windows.h>
#include <string>
#include <vector>

// A "chord" (multi-key) shortcut, e.g. Ctrl+M followed by M.
// A plain Win32 ACCELERATORS table cannot express that, so chords live in a
// small C++ table (see ChordAccel.cpp) which is used both to match the keys at
// run time and to build the menu text shown by CAccel2Menu.
//
// mod1/mod2 use the same flags as an .rc ACCELERATORS: FSHIFT, FCONTROL, FALT
struct ChordAccel {
	BYTE vk1;	// 1st key: virtual-key code, e.g. 'M' (same as VK_M)
	BYTE mod1;	// 1st key: FSHIFT | FCONTROL | FALT
	BYTE vk2;	// 2nd key: virtual-key code
	BYTE mod2;	// 2nd key: FSHIFT | FCONTROL | FALT
	UINT cmd;	// command id, posted as WM_COMMAND
};

// The chord table (defined in ChordAccel.cpp).
const std::vector<ChordAccel>& chordAccels();

// Displays a chord, e.g. L"Ctrl+M, M".
std::wstring chordAccel2str( const ChordAccel& ca );

// Multi-key shortcut support. Call it from the main message loop, before
// TranslateAccelerator(). Returns true when msg has been consumed and must not
// be translated/dispatched.
bool TranslateChordAccel( MSG& msg );
