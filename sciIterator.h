#pragma once

class CSciIterator : public iterator<random_access_iterator_tag,char> 
{
	CSciWrapper* m_pSci;
	int	m_pos;
public:
	CSciIterator( CSciWrapper* pSci=NULL, int pos=0 ) 
		: m_pSci(pSci), m_pos(pos) {}
	CSciIterator( const CSciIterator& o ) 
		: m_pSci(o.m_pSci), m_pos(o.m_pos) {}

	int getPos() const 
		{ return m_pos; }
	int getStyleAt() const
		{ ASSERT(m_pSci); return m_pSci->GetStyleAt(m_pos); }

	char operator*() const 
		{ ASSERT(m_pSci); return m_pSci->GetCharAt(m_pos); }

//	iterator functions
	CSciIterator& operator++() 
		{ m_pos++; return *this; }
	CSciIterator& operator+=(int dist) 
		{ m_pos+=dist; return *this; }

	CSciIterator& operator--() 
		{ m_pos--; return *this; }
	int operator-(const CSciIterator& o) 
		{ ASSERT(m_pSci==o.m_pSci); return m_pos - o.m_pos; }

//	relational operators		
	bool operator==(const CSciIterator& o) const 
		{ return (m_pSci == o.m_pSci) && (m_pos == o.m_pos); }
	bool operator!=(const CSciIterator& o) const 
		{ return !((*this) == o); }          
	bool operator<(const CSciIterator& o) const	
		{ ASSERT( m_pSci == o.m_pSci ); return m_pos < o.m_pos; }
};

inline
string GetTextRange( const pair<CSciIterator,CSciIterator>& ii )
{
	string ret;
	for( CSciIterator i=ii.first; i!=ii.second; ++i )
		ret += *i;
	return ret;
}
