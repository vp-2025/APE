#include "ShellContextMenu.h"
using namespace std;
#include "shared/str.h"
#include "shared/platform.h"

#define MIN_ID 1
#define MAX_ID 10000

IContextMenu2 * g_IContext2 = NULL;
IContextMenu3 * g_IContext3 = NULL;

CShellContextMenu::CShellContextMenu()
{
	m_psfFolder = NULL;
	m_pidlArray = NULL;
	m_hMenu = NULL;
}

CShellContextMenu::~CShellContextMenu()
{
	// free all allocated datas
	if (m_psfFolder && bDelete)
		m_psfFolder->Release();
	m_psfFolder = NULL;
	FreePIDLArray(m_pidlArray);
	m_pidlArray = NULL;

	if( m_hMenu )
		DestroyMenu(m_hMenu);
}

// this functions determines which version of IContextMenu is avaibale for those objects (always the highest one)
// and returns that interface
BOOL CShellContextMenu::GetContextMenu(void ** ppContextMenu, int & iMenuType)
{
	*ppContextMenu = NULL;
	LPCONTEXTMENU icm1 = NULL;
	
	// first we retrieve the normal IContextMenu interface (every object should have it)
	m_psfFolder->GetUIObjectOf (NULL, nItems, (LPCITEMIDLIST *) m_pidlArray, IID_IContextMenu, NULL, (void**) &icm1);

	if(icm1)
	{	// since we got an IContextMenu interface we can now obtain the higher version interfaces via that
		if (icm1->QueryInterface (IID_IContextMenu3, ppContextMenu) == NOERROR)
			iMenuType = 3;
		else if (icm1->QueryInterface (IID_IContextMenu2, ppContextMenu) == NOERROR)
			iMenuType = 2;

		if (*ppContextMenu) 
			icm1->Release(); // we can now release version 1 interface, cause we got a higher one
		else  {	
			iMenuType = 1;
			*ppContextMenu = icm1;	// since no higher versions were found
		}							// redirect ppContextMenu to version 1 interface
	} else
		return (FALSE);	// something went wrong
	
	return (TRUE); // success
}

WNDPROC g_OldWndProc=NULL;

LRESULT CALLBACK CShellContextMenu::HookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{ 
	case WM_MENUCHAR:	// only supported by IContextMenu3
		if (g_IContext3)
		{
			LRESULT lResult = 0;
			g_IContext3->HandleMenuMsg2 (message, wParam, lParam, &lResult);
			return (lResult);
		}
		break;

	case WM_DRAWITEM:
	case WM_MEASUREITEM:
		if (wParam) 
			break; // if wParam != 0 then the message is not menu-related
  
	case WM_INITMENUPOPUP:
		if (g_IContext2)
			g_IContext2->HandleMenuMsg (message, wParam, lParam);
		if( g_IContext3 )
			g_IContext3->HandleMenuMsg (message, wParam, lParam);
		return (message == WM_INITMENUPOPUP ? 0 : TRUE); // inform caller that we handled WM_INITPOPUPMENU by ourself
		break;

	default:
		break;
	}

	// call original WndProc of window to prevent undefined bevhaviour of window
	return ::CallWindowProc(g_OldWndProc, hWnd, message, wParam, lParam);
}

UINT CShellContextMenu::ShowContextMenu(HWND hWnd, POINT pt)
{
	for( int i=0; i<nItems; i++ )
		if( !m_pidlArray[i] )
			return 0;

	int iMenuType = 0;	// to know which version of IContextMenu is supported
	LPCONTEXTMENU pContextMenu;	// common pointer to IContextMenu and higher version interface
   
	if( !GetContextMenu ((void**) &pContextMenu, iMenuType) )	
		return (0);	// something went wrong

	if( !m_hMenu )
	{
		DestroyMenu(m_hMenu);
		m_hMenu = NULL;
		m_hMenu = CreatePopupMenu();
	}

	// lets fill the our popupmenu  
	pContextMenu->QueryContextMenu(m_hMenu, GetMenuItemCount(m_hMenu), MIN_ID, MAX_ID, CMF_NORMAL|CMF_EXPLORE );
 
	// subclass window to handle menurelated messages in CShellContextMenu 
	if (iMenuType > 1)	// only subclass if its version 2 or 3
	{
		g_OldWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) HookWndProc);
		if( iMenuType == 2 )
			g_IContext2 = (LPCONTEXTMENU2)pContextMenu;
		if( iMenuType == 3 )
			g_IContext3 = (LPCONTEXTMENU3)pContextMenu;
	}

	UINT idCommand = TrackPopupMenu(m_hMenu, TPM_RETURNCMD | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);

	if( g_OldWndProc ) // unsubclass
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)g_OldWndProc);

	if (idCommand >= MIN_ID && idCommand <= MAX_ID)	// see if returned idCommand belongs to shell menu entries
	{
		InvokeCommand (pContextMenu, idCommand - MIN_ID);	// execute related command
		idCommand = 0;
	}
	
	pContextMenu->Release();
	g_IContext2 = NULL;
	g_IContext3 = NULL;

	return (idCommand);
}

void CShellContextMenu::InvokeCommand (LPCONTEXTMENU pContextMenu, UINT idCommand)
{
	CMINVOKECOMMANDINFO cmi = {0};
	cmi.cbSize = sizeof (CMINVOKECOMMANDINFO);
	cmi.lpVerb = (LPSTR) MAKEINTRESOURCE (idCommand);
	cmi.nShow = SW_SHOWNORMAL;
	
	pContextMenu->InvokeCommand (&cmi);
}

void CShellContextMenu::SetObject(const string& sObject)
{
	vector<string> vObjects;
	vObjects.push_back(sObject);
	SetObjects(vObjects);
}

void CShellContextMenu::SetObjects(const vector<string>& vObjects)
{
	if( vObjects.empty() ) return;

	// free all allocated datas
	if (m_psfFolder && bDelete)
		m_psfFolder->Release();
	m_psfFolder = NULL;
	FreePIDLArray (m_pidlArray);
	m_pidlArray = NULL;
	
	// get IShellFolder interface of Desktop (root of shell namespace)
	IShellFolder * psfDesktop = NULL;
	SHGetDesktopFolder(&psfDesktop);	// needed to obtain full qualified pidl

	// ParseDisplayName creates a PIDL from a file system path relative to the IShellFolder interface
	// but since we use the Desktop as our interface and the Desktop is the namespace root
	// that means that it's a fully qualified PIDL, which is what we need
	LPITEMIDLIST pidl = NULL;
	
/*	OLECHAR * olePath = NULL;
	int len = vObjects[0].length()+1;
	olePath = (OLECHAR *)calloc(len, sizeof(OLECHAR));
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, vObjects[0].c_str(), -1, olePath, len );	
	psfDesktop->ParseDisplayName(NULL, 0, olePath, NULL, &pidl, NULL);
	free(olePath); */
	psfDesktop->ParseDisplayName(NULL, 0, (wchar_t*)utf2w(vObjects[0]).c_str(), NULL, &pidl, NULL);

	// now we need the parent IShellFolder interface of pidl, and the relative PIDL to that interface
	LPITEMIDLIST pidlItem = NULL;	// relative pidl
	SHBindToParentEx (pidl, IID_IShellFolder, (void **) &m_psfFolder, NULL);
	free(pidlItem);
	// get interface to IMalloc (need to free the PIDLs allocated by the shell functions)
	LPMALLOC lpMalloc = NULL;
	SHGetMalloc (&lpMalloc);
	lpMalloc->Free (pidl);

	// now we have the IShellFolder interface to the parent folder specified in the first element in strArray
	// since we assume that all objects are in the same folder (as it's stated in the MSDN)
	// we now have the IShellFolder interface to every objects parent folder
	
	IShellFolder * psfFolder = NULL;
	nItems = vObjects.size();
	for (int i = 0; i < nItems; i++)
	{
/*		int len = vObjects[i].length()+1;
		olePath = (OLECHAR *)calloc(len, sizeof (OLECHAR));
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, vObjects[i].c_str(), -1, olePath, len); 
		psfDesktop->ParseDisplayName (NULL, 0, olePath, NULL, &pidl, NULL);
		free (olePath); */
		psfDesktop->ParseDisplayName (NULL, 0, (wchar_t*)utf2w(vObjects[i]).c_str(), NULL, &pidl, NULL);

		m_pidlArray = (LPITEMIDLIST *) realloc (m_pidlArray, (i + 1) * sizeof (LPITEMIDLIST));
		// get relative pidl via SHBindToParent
		SHBindToParentEx (pidl, IID_IShellFolder, (void **) &psfFolder, (LPCITEMIDLIST *) &pidlItem);
		m_pidlArray[i] = CopyPIDL(pidlItem);	// copy relative pidl to pidlArray
		free(pidlItem);
		lpMalloc->Free(pidl);		// free pidl allocated by ParseDisplayName
		if( psfFolder )
			psfFolder->Release();
	}
	lpMalloc->Release();
	psfDesktop->Release();

	bDelete = TRUE;	// indicates that m_psfFolder should be deleted by CShellContextMenu
}

void CShellContextMenu::FreePIDLArray(LPITEMIDLIST *pidlArray)
{
	if (!pidlArray)
		return;

	int iSize = _msize (pidlArray) / sizeof (LPITEMIDLIST);

	for (int i = 0; i < iSize; i++)
		free (pidlArray[i]);
	free (pidlArray);
}

LPITEMIDLIST CShellContextMenu::CopyPIDL(LPCITEMIDLIST pidl, int cb)
{
	if( cb==-1 )
		cb = GetPIDLSize (pidl); // Calculate size of list.
	if( !cb )
		return NULL;

    auto pidlRet = (LPITEMIDLIST)calloc(cb + sizeof(USHORT), sizeof(BYTE));
    if( pidlRet )
		CopyMemory(pidlRet, pidl, cb);

    return pidlRet;
}

UINT CShellContextMenu::GetPIDLSize(LPCITEMIDLIST pidl)
{  
	if (!pidl) 
		return 0;
	int nSize = 0;
	auto pidlTemp = (LPITEMIDLIST) pidl;
	while (pidlTemp->mkid.cb)
	{
		nSize += pidlTemp->mkid.cb;
		pidlTemp = (LPITEMIDLIST) (((LPBYTE) pidlTemp) + pidlTemp->mkid.cb);
	}
	return nSize;
}

// this is workaround function for the Shell API Function SHBindToParent
// SHBindToParent is not available under Win95/98
HRESULT CShellContextMenu::SHBindToParentEx (LPCITEMIDLIST pidl, REFIID riid, VOID **ppv, LPCITEMIDLIST *ppidlLast)
{
	HRESULT hr = 0;
	if (!pidl || !ppv)
		return E_POINTER;
	
	int nCount = GetPIDLCount (pidl);
	if (nCount == 0)	// desktop pidl of invalid pidl
		return E_POINTER;

	IShellFolder * psfDesktop = NULL;
	SHGetDesktopFolder (&psfDesktop);
	if (nCount == 1)	// desktop pidl
	{
		if ((hr = psfDesktop->QueryInterface(riid, ppv)) == S_OK)
		{
			if (ppidlLast) 
				*ppidlLast = CopyPIDL (pidl);
		}
		psfDesktop->Release ();
		return hr;
	}

	LPBYTE pRel = GetPIDLPos (pidl, nCount - 1);
	LPITEMIDLIST pidlParent = NULL;
	pidlParent = CopyPIDL (pidl, pRel - (LPBYTE) pidl);
	IShellFolder * psfFolder = NULL;
	
	if ((hr = psfDesktop->BindToObject (pidlParent, NULL, __uuidof (psfFolder), (void **) &psfFolder)) != S_OK)
	{
		free (pidlParent);
		psfDesktop->Release ();
		return hr;
	}
	if ((hr = psfFolder->QueryInterface (riid, ppv)) == S_OK)
	{
		if (ppidlLast)
			*ppidlLast = CopyPIDL ((LPCITEMIDLIST) pRel);
	}
	free (pidlParent);
	psfFolder->Release ();
	psfDesktop->Release ();
	return hr;
}

LPBYTE CShellContextMenu::GetPIDLPos (LPCITEMIDLIST pidl, int nPos)
{
	if (!pidl)
		return 0;
	int nCount = 0;
	
	BYTE * pCur = (BYTE *) pidl;
	while (((LPCITEMIDLIST) pCur)->mkid.cb)
	{
		if (nCount == nPos)
			return pCur;
		nCount++;
		pCur += ((LPCITEMIDLIST) pCur)->mkid.cb;	// + sizeof(pidl->mkid.cb);
	}
	if (nCount == nPos) 
		return pCur;
	return NULL;
}

int CShellContextMenu::GetPIDLCount (LPCITEMIDLIST pidl)
{
	if (!pidl)
		return 0;

	int nCount = 0;
	BYTE*  pCur = (BYTE *) pidl;
	while (((LPCITEMIDLIST) pCur)->mkid.cb)
	{
		nCount++;
		pCur += ((LPCITEMIDLIST) pCur)->mkid.cb;
	}
	return nCount;
}

/////////////////////////////////////////////////////////////////////////////

void OpenFolder( const string& sFolder, const string& sFile ) {
	ITEMIDLIST *pDir = ILCreateFromPathW(utf2w(sFolder).c_str());
	ITEMIDLIST *pFile = nullptr;
	if( !sFile.empty() )
		pFile = ILCreateFromPathW(utf2w(sFile).c_str());
	const ITEMIDLIST* selection[] = {pFile};
	SHOpenFolderAndSelectItems(pDir, 1, selection, 0);
	ILFree(pDir);
	if( pFile!=nullptr )
		ILFree(pFile);
}

/////////////////////////////////////////////////////////////////////////////

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData) {
	TCHAR szDir[MAX_PATH]; // todo unicode

	switch( uMsg ) {
		case BFFM_INITIALIZED: {
			// WParam is TRUE since you are passing a path.
			// It would be FALSE if you were passing a pidl.
			if( pData && ((TCHAR*) pData)[0] ) {
				SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM) pData);
			} else {
				if( GetCurrentDirectory(sizeof(szDir) / sizeof(TCHAR), szDir) ) {
					SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM) szDir);
				}
			}
			break;
		}
		case BFFM_SELCHANGED: {
			// Set the status window to the currently selected path.
			if( SHGetPathFromIDList((LPITEMIDLIST) lp, szDir) ) {
				SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM) szDir);
			}
			break;
		}
		default:
			break;
	}
	return 0;
}

void _SHFree(void* p) {
	IMalloc* pm;
	SHGetMalloc(&pm);
	if( pm ) {
		pm->Free(p);
		pm->Release();
	}
}

bool BrowseForFolder(HWND hWnd, string& sPath) {
	string str = sPath;
	delTailSlash(str);
	wstring wstr = utf2w(str);

	BROWSEINFOW bi {};
	bi.hwndOwner = hWnd;
//	bi.pszDisplayName = szDisplayBuf;
//	bi.lpszTitle = szTitle;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = (LPARAM)wstr.c_str();

	LPITEMIDLIST pIdl = SHBrowseForFolderW(&bi);

	if( pIdl ) {
		wstrbuf sBuf(MAX_PATH);
		SHGetPathFromIDListW(pIdl, sBuf.buf());
		sPath = w2utf(sBuf.str()); // no len
		_SHFree(pIdl);
		addTailSlash(sPath);
		return true;
	}

	return false;
}