#include "mru.h"
#include "options.h"
#include "shared/win.h" // CRegistry
#include "shared/str.h"
#include "shared/platform.h"
#include "lang.h"
#include <cctype>
using namespace std;

void CMRU::Load()
{
	CRegistry reg;
	if( !reg.Open(g_hKey,string(g_szPath)+szKey) )
		return;
	int cnt = reg.ReadInt("count");
	for( int i=0; i<cnt; i++ )
		m_list.push_back( reg.ReadStr("item"+itoa(i) ) );
}

void CMRU::Save()
{
	CRegistry reg;
	if( !reg.Create(g_hKey,string(g_szPath)+szKey) )
		return;
	reg.WriteInt( "count", m_list.size() );
	for( unsigned i=0; i<m_list.size(); i++ )
		reg.WriteStr( "item"+itoa(i), m_list[i] );
}

void LowerCase1stChar(string& str)
{
	if (!str.empty())
		str[0] = std::tolower(str[0]);
}

void CMRU::Add( const string& s1 )
{
	if( m_bPause ) return;
	string s = s1;
	EnsureFilePath( s );
	LowerCase1stChar(s);
	for( size_t i=0; i<m_list.size(); )
		if( equalsToIC(m_list[i], s) )
			Remove( i );
		else
			++i;
	m_list.push_front(s);
	while( (int)m_list.size()>g_options.iMRUSize )
		m_list.pop_back();
}

void CMRU::Remove( int idx )
{
	if( idx<0 || idx>=(int)m_list.size() )
		return;
	m_list.erase( m_list.begin()+idx );
}

void CMRU::UpdateMenu( HMENU hMenu )
{
	for( int i=m_cmdFirst; i<=m_cmdLast; i++ )
		DeleteMenu( hMenu, i, MF_BYCOMMAND );
	string sCur = toLowerVal(GetCurDir());
	addTailSlash(sCur);
	int cnt = GetMenuItemCount(hMenu);
	for( unsigned i=0; i<m_list.size(); i++ )
	{
		string sFile =  m_list[i];
		// LowerCase1stChar(sFile);
		InsertMenuW( hMenu, cnt+i, MF_BYPOSITION|MF_STRING, m_cmdFirst+i, utf2w(sFile).c_str() );
	}
}

string CMRU::GetFile(int idx) {
	if( idx<0 || idx>=(int)m_list.size() )
		return {};
	return m_list[idx]; 
}

string CMRU::GetFileWithRemove(int idx, bool bAsk) 
{
	string s = GetFile(idx);
	if( s.empty() || !IsFileExists(s) )
	{
		bool bDel = true;
		if( bAsk )
			bDel = MessageBoxW(NULL,
				tr(L"File does not exist.\nDo you want to remove it from list?").c_str(),
				tr(L"Confirm").c_str(), MB_YESNO)==IDYES;
		if( bDel )
			Remove(idx);
		return {};
	}
	return s;
}

////////////////////////////

void CPinned::doPin(const string& s1)
{
	string s = s1;
	EnsureFilePath( s );

	int idx=-1;
	for( size_t i=0; i<m_list.size(); i++ )
		if( m_list[i]==s )
		{
			idx = i; break;
		}
	if( idx==-1 )
		m_list.push_back(s);
	else
		m_list.erase( m_list.begin()+idx, m_list.begin()+idx+1 );
}

bool CPinned::isPinned(const string& s1)
{
	string s = s1;
	EnsureFilePath( s );

	for( size_t i=0; i<m_list.size(); i++ )
		if( m_list[i]==s )
			return true;
	return false;
}
