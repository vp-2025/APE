#include "stdafx.h" 
#include "progress.h"
#include "shared/str.h"

CProgress::CProgress( const char* szTitle, int min, int max )
	: m_szTitle(szTitle), m_min(min), m_max(max), m_dw(0), m_pTaskbarList(NULL), m_btn(NULL)
{
	int w = 150;
	int h = 30;
	CRect r;
	GetWindowRect(g_hMainWnd, &r);
	m_btn = new CButton();
	m_btn->Create( g_hMainWnd, r.left+(r.width()-w)/2, r.top + (r.height()-h)/2, w, h, 0, 0, WS_CHILD|WS_VISIBLE|WS_POPUP );

	CoCreateInstance( CLSID_TaskbarList, NULL, CLSCTX_ALL, IID_ITaskbarList3, (void**)&m_pTaskbarList );
	if( m_pTaskbarList ) {
		m_pTaskbarList->SetProgressState( g_hMainWnd, TBPF_NORMAL );
		m_pTaskbarList->SetProgressValue( g_hMainWnd, m_min, m_max );
	}
}

CProgress::~CProgress()
{
	if( m_btn ) {
		m_btn->Destroy();
		delete m_btn;
	}
	if( m_pTaskbarList ) {
		m_pTaskbarList->SetProgressState( g_hMainWnd, TBPF_NOPROGRESS );
		m_pTaskbarList->Release();
	}
}

void CProgress::next( int pos )
{
	if( GetTickCount()-m_dw<100 ) return;
	m_dw = GetTickCount();
	if( m_btn )
		m_btn->SetText( std::string()<<m_szTitle<<' '<< (int)((pos-m_min)*100.0/(m_max-m_min)) <<'%' );
	if( m_pTaskbarList )
		m_pTaskbarList->SetProgressValue( g_hMainWnd, pos, m_max );
}
