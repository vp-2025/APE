#pragma once

#include <shobjidl.h>
#include <shlobj.h>
#include <string>
#include <vector>

class CShellContextMenu  
{
public:
	CShellContextMenu();
	virtual ~CShellContextMenu();

	void SetObject(const std::string& sObject);
	void SetObjects(const std::vector<std::string>& vObjects);
	UINT ShowContextMenu(HWND hWnd, POINT pt);
private:
	int nItems;
	BOOL bDelete;
	HMENU m_hMenu;
	IShellFolder * m_psfFolder;
	LPITEMIDLIST * m_pidlArray;	
	
	void InvokeCommand(LPCONTEXTMENU pContextMenu, UINT idCommand);
	BOOL GetContextMenu(void ** ppContextMenu, int & iMenuType);
	HRESULT SHBindToParentEx(LPCITEMIDLIST pidl, REFIID riid, VOID **ppv, LPCITEMIDLIST *ppidlLast);
	static LRESULT CALLBACK HookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void FreePIDLArray(LPITEMIDLIST * pidlArray);
	LPITEMIDLIST CopyPIDL(LPCITEMIDLIST pidl, int cb = -1);
	UINT GetPIDLSize(LPCITEMIDLIST pidl);
	LPBYTE GetPIDLPos(LPCITEMIDLIST pidl, int nPos);
	int GetPIDLCount(LPCITEMIDLIST pidl);
};

void OpenFolder( const std::string& sFolder, const std::string& sFile={} );
bool BrowseForFolder(HWND hWnd, std::string& sPath);