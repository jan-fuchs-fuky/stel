/*
 *  CDeinterlace.h
 *  CDeinterlace
 *
 *  Created by Scott Streit on 2/9/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef _ARC_CDEINTERLACE_H_
#define _ARC_CDEINTERLACE_H_

//#ifdef __APPLE__
//#pragma GCC visibility push( default )
//#endif


#include <stdexcept>
#include <string>
#include "DllMain.h"



namespace arc
{
	// This class is exported from the CDeinterlace.dll
	class CDEINTERLACE_API CDeinterlace
	{
	public:
		CDeinterlace();
		
		void RunAlg( void *data, int rows, int cols, int algorithm, int arg = -1 );
		
		const static int DEINTERLACE_NONE        = 0;
		const static int DEINTERLACE_PARALLEL    = 1;
		const static int DEINTERLACE_SERIAL      = 2;
		const static int DEINTERLACE_CCD_QUAD    = 3;
		const static int DEINTERLACE_IR_QUAD     = 4;
		const static int DEINTERLACE_CDS_IR_QUAD = 5;
		const static int DEINTERLACE_HAWAII_RG   = 6;
		const static int DEINTERLACE_STA1600	 = 7;
		
	private:
		unsigned short *new_iptr;
		
		void Parallel( unsigned short *data, int rows, int cols );
		void Serial( unsigned short *data, int rows, int cols );
		void CCDQuad( unsigned short *data, int rows, int cols );
		void IRQuad( unsigned short *data, int rows, int cols );
		void IRQuadCDS( unsigned short *data, int rows, int cols );
		void HawaiiRG( unsigned short *data, int rows, int cols, int arg );
		void STA1600( unsigned short *data, int rows, int cols );
		void ThrowException( std::string sMethodName, std::string sMsg );
	};
	
}	// end namespace


//#ifdef __APPLE__
//#pragma GCC visibility pop
//#endif


#endif
