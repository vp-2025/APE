#include "ChordAccel.h"
#include "AccelMenu.h"	// accel2str()
#include "resource.h"
using namespace std;

extern HWND g_hMainWnd;

///////////////////////////////////////////////////////////////////////////////
//
// Multi-key ("chord") shortcuts: two consecutive key presses, e.g. Ctrl+M, M.
//
// This table is the single source of truth. CAccel2Menu shows the entries in
// the menus and TranslateChordAccel() below matches them at run time.
//
//   1st key + 2nd key -> command
//
// Both keys are matched exactly, so { 'M', FCONTROL, 'M', 0 } means:
// press Ctrl+M, release Ctrl, then press M. To keep Ctrl held for the second
// press use FCONTROL as mod2.
//
// The scheme used for the folding commands:
//   no extra modifier - the normal variants
//   SHIFT             - the "... All" variants
//   CONTROL           - the "... Children" variants
//
static const ChordAccel s_chords[] = {
	{'M', FCONTROL, 'M', FCONTROL, ID_FOLDING_TOGGLE},
	{'M', FCONTROL, 'S', FCONTROL, ID_FOLDING_COLLAPSE},
	{'M', FCONTROL, 'E', FCONTROL, ID_FOLDING_EXPAND},

	{'M', FCONTROL, 'A', FCONTROL, ID_FOLDING_COLLAPSEALL},
	{'M', FCONTROL, 'X', FCONTROL, ID_FOLDING_EXPANDALL},
};

static const size_t s_chordCount = sizeof(s_chords)/sizeof(s_chords[0]);

const vector<ChordAccel>& chordAccels() {
	static const vector<ChordAccel> v( s_chords, s_chords + s_chordCount );
	return v;
}

wstring chordAccel2str( const ChordAccel& ca ) {
	return accel2str( ca.mod1|FVIRTKEY, ca.vk1 ) + L", " + accel2str( ca.mod2|FVIRTKEY, ca.vk2 );
}

///////////////////////////////////////////////////////////////////////////////

// the pending 1st key of a chord (nullptr - nothing pending)
static const ChordAccel* s_pending = nullptr;

static bool isModifierKey( BYTE vk ) {
	switch( vk ) {
		case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
		case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
		case VK_MENU: case VK_LMENU: case VK_RMENU:
		case VK_LWIN: case VK_RWIN:
			return true;
	}
	return false;
}

// currently held modifiers, in the ACCELERATORS flags notation
static BYTE currentMods() {
	BYTE mod = 0;
	if( GetKeyState(VK_SHIFT)   & 0x8000 ) mod |= FSHIFT;
	if( GetKeyState(VK_CONTROL) & 0x8000 ) mod |= FCONTROL;
	if( GetKeyState(VK_MENU)    & 0x8000 ) mod |= FALT;
	return mod;
}

bool TranslateChordAccel( MSG& msg ) {
	// a click or a focus change cancels an unfinished chord
	switch( msg.message ) {
		case WM_LBUTTONDOWN: case WM_MBUTTONDOWN: case WM_RBUTTONDOWN:
		case WM_XBUTTONDOWN: case WM_MOUSEWHEEL: case WM_KILLFOCUS:
			s_pending = nullptr;
			return false;
		case WM_KEYDOWN: case WM_SYSKEYDOWN:
			break;
		default:
			return false;
	}

	BYTE vk = (BYTE)msg.wParam;
	if( isModifierKey(vk) )
		return false;	// a modifier alone neither starts nor cancels a chord

	BYTE mods = currentMods();

	if( s_pending ) {
		// 2nd key of a chord?
		const ChordAccel* p = s_pending;
		s_pending = nullptr;

		if( vk == VK_ESCAPE )
			return true;	// Esc cancels the chord and is swallowed

		for( const ChordAccel& ca : chordAccels() )
			if( ca.vk1==p->vk1 && ca.mod1==p->mod1 && ca.vk2==vk && ca.mod2==mods ) {
				PostMessage( g_hMainWnd, WM_COMMAND, MAKEWPARAM(ca.cmd,0), 0 );
				return true;
			}

		return false;	// no match - cancel and let the key through
	}

	// 1st key of a chord?
	for( const ChordAccel& ca : chordAccels() )
		if( ca.vk1==vk && ca.mod1==mods ) {
			s_pending = &ca;
			return true;	// swallow the 1st key and wait for the 2nd one
		}

	return false;
}
