#pragma once

#include <shobjidl.h>
#include "shared/win.h"

class CButton;

class CProgress
{
	ITaskbarList3* m_pTaskbarList;
	CButton* m_btn;
	int m_min, m_max;
	const char* m_szTitle;
	unsigned m_dw;
public:
	CProgress( const char* szTitle, int min, int max );
	~CProgress();
	void next( int pos );
	void next( int pos, int max ) { m_max=max; next(pos); }
};
