#pragma once

#include <windows.h>
#include <vector>
#include <deque>
#include <string>

class CMRU
{
	int m_cmdFirst, m_cmdLast;
	bool m_bPause;
protected:
	std::deque<std::string> m_list;
	const char* szKey;
public:
	CMRU( int cmdFirst, int cmdLast) { m_cmdFirst=cmdFirst; m_cmdLast=cmdLast; m_bPause=false; szKey="MRU"; }
	void Load();
	void Save();
	void Add( const std::string& );
	void Remove( int idx );
	void UpdateMenu( HMENU hMenu );
	int GetSize() { return m_list.size(); }
	std::string GetFile(int idx);
	std::string GetFileWithRemove(int idx, bool bAsk);
	void Clear() { m_list.clear(); }

	void Pause() { m_bPause=true; }
	void Continue() { m_bPause=false; }
};

class CPinned : public CMRU
{
public:
	CPinned( int cmdFirst, int cmdLast) : CMRU(cmdFirst, cmdLast) { szKey="Pin"; }

	void doPin(const std::string& s);
	bool isPinned(const std::string& s);
};