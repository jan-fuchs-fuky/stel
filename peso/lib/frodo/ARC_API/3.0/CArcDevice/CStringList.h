#ifndef _ARC_CSTRINGLIST_H_
#define _ARC_CSTRINGLIST_H_

#include <vector>
#include <string>

namespace arc
{

	class CStringList
	{
		public:
			void Add( std::string const &sElem );
			void Clear();
			bool Empty();

			std::string  ToString();

			std::string& At( int dIndex );
			int Length();

			CStringList& operator<<( std::string const &sElem );
			CStringList& operator+=( CStringList& anotherList );

		private:
			std::vector<std::string> m_vList;
			std::string* m_psArray;
	};

}	// end namespace

#endif
