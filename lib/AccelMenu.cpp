#include "AccelMenu.h"
using namespace std;
#include "shared/str.h"

extern HINSTANCE g_hInst;

wstring accel2str( BYTE virt, WORD key )
{
	wstring ret;
	if( virt&FCONTROL )
		ret += L"Ctrl+";
	if( virt&FALT )
		ret += L"Alt+";
	if( virt&FSHIFT )
		ret += L"Shift+";
	if( virt&FVIRTKEY ) {
		UINT vkey = MapVirtualKey(key, 0)<<16;
		wstrbuf sBuf(MAX_PATH);
		int len = GetKeyNameTextW(vkey, sBuf.buf(), sBuf.size());
		ret += sBuf.str(len); // len without ending zero
	} else
		ret += char(key);
	return ret;
}

CAccel2Menu::CAccel2Menu( UINT nAccelID )
{
	HACCEL hAccel = LoadAccelerators(g_hInst, MAKEINTRESOURCE(nAccelID));
	int nAccel = CopyAcceleratorTable( hAccel, nullptr, 0 );
	if( nAccel ) {
		auto* pAccel = new ACCEL[nAccel];
		CopyAcceleratorTable( hAccel, pAccel, nAccel );
		for( int i=0; i<nAccel; i++ )
			m[pAccel[i].cmd] = accel2str( pAccel[i].fVirt, pAccel[i].key );
		delete[] pAccel;
	}
}

void CAccel2Menu::proceedMenu( HMENU hMenu )
{
	int cnt = GetMenuItemCount(hMenu);
	for( int pos=0; pos<cnt; ++pos )
	{
		HMENU hM = GetSubMenu(hMenu, pos);
		if( hM )
			proceedMenu( hM );
		else {
			wstrbuf sBuf(MAX_PATH);
			MENUITEMINFOW mii{};
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_ID | MIIM_STRING;
			mii.dwTypeData = sBuf.buf();
			mii.cch = sBuf.size();
			if( !GetMenuItemInfoW( hMenu, pos, TRUE, &mii ) )
				continue;
			wstring strO(sBuf.str()); // no len
			wstring str(strO);
			size_t p = str.find('\t');
			if( p!=string::npos )
				str.erase(p);
			wstring sAccel = m[mii.wID];
			if( !sAccel.empty() ) {
				str += '\t';
				str += sAccel;
			}
			if( str!=strO ) {
				sBuf.set(str);
//				mii.dwTypeData && mii.cch - are same
				SetMenuItemInfoW( hMenu, pos, TRUE, &mii );
			}
		}
	}
}
