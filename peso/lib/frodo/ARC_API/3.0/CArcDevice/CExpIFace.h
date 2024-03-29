#ifndef _ARC_CEXPIFACE_H_
#define _ARC_CEXPIFACE_H_

#include "DllMain.h"


namespace arc
{

	class CARCDEVICE_API CExpIFace   // Expose Interface Class
	{
		public:
			virtual ~CExpIFace() {}

			virtual void ExposeCallback( float fElapsedTime ) = 0;
			virtual void ReadCallback( int dPixelCount ) = 0;
	};

}	// end namespace


#endif	// _CEXPIFACE_H_
