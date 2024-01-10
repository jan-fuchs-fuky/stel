#include "CStringList.h"

using namespace std;
using namespace arc;


void CStringList::Add( std::string const &sElem )
{
	 m_vList.push_back( sElem );
}

void CStringList::Clear()
{
	  m_vList.clear();
}

bool CStringList::Empty()
{
	return m_vList.empty();
}

std::string CStringList::ToString()
{
	std::vector<std::string>::iterator it;
	std::string sTemp;

	for ( it = m_vList.begin() ; it != m_vList.end() ; it++ )
	{
		sTemp += std::string( *it );
		sTemp += std::string( "\n" );
	}

	return sTemp;
}

std::string& CStringList::At( int dIndex )
{
	return m_vList.at( dIndex );
}

int CStringList::Length()
{
	return int( m_vList.size() );
}

// Operator overloaded using a member function
CStringList& CStringList::operator<<( std::string const &sElem )
{
	m_vList.push_back( sElem );
	return *this;
}

CStringList& CStringList::operator+=( CStringList& anotherList )
{
	for ( int i=0; i<anotherList.Length(); i++ )
	{
		m_vList.push_back( anotherList.At( i ) );
	}

	return *this;
}
