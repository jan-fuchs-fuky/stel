//
// Log.h : Defines a string message logging class
//

#ifndef _ARC_CLOG_H_
#define _ARC_CLOG_H_

#include <queue>
#include <string>
#include <sstream>
#include <cstdarg>
#include "DllMain.h"


namespace arc
{
	class CARCDEVICE_API CLog
	{
	public:
		void  SetMaxSize( int dSize );

		void  Put( const char *szFmt, ... );

		char* GetNext();
		char* GetLast();
		int   GetLogCount();
		bool  Empty();

		void  SelfTest();

	private:
		static std::queue<std::string>::size_type Q_MAX;

		char m_szBuffer[ 256 ];
		std::queue<std::string> m_sQueue;
	};

}	// end namespace


#endif
