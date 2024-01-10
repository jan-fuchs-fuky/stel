// CDeinterlace.cpp : Defines the exported functions for the DLL application.
//

#ifdef WIN32
#include <windows.h>
#endif

#include <sstream>
#include <stdexcept>
#include <cstring>
#include "CDeinterlace.h"

using namespace std;
using namespace arc;

// +----------------------------------------------------------------------------
// | Default constructor
// +----------------------------------------------------------------------------
CDeinterlace::CDeinterlace()
{
	new_iptr = NULL;
}

// +----------------------------------------------------------------------------
// | RunAlg - Deinterlace Image
// +----------------------------------------------------------------------------
// |
// | PARAMETERS:	mem_fd   File descriptor that references the image.
// | rows	 The number of rows in the image.
// | cols	 The number of columns in the image.
// | 
// | DESCRIPTION:	The deinterlacing algorithms work on the principle that the
// | ccd/ir array will read out the data in a predetermined order depending on
// | the type of readout being implemented.  Here's how they look:     			
// | 
// | split-parallel          split-serial              quad CCD                quad IR          	
// | ----------------       ----------------        ----------------        ----------------     	
// | |     1  ------->|     |        |------>|      |<-----  |  ---->|      | -----> | ----> |    	
// | |                |     |        |   1   |      |   3    |   2   |      |   0    |   1   |    	
// | |                |     |        |       |      |        |       |      |        |       |    	
// | |_______________ |     |        |       |      |________|_______|      |________|_______|    	
// | |                |     |        |       |      |        |       |      |        |       |    	
// | |                |     |        |       |      |        |       |      |        |       |    	
// | |                |     |   0    |       |      |   0    |   1   |      |   3    |   2   |    	
// | |<--------  0    |     |<------ |       |      |<-----  |  ---->|      | -----> | ----> |    	
// |  ----------------       ----------------        ----------------        ----------------     	
// | 
// | CDS quad IR          	
// | ----------------     	
// | | -----> | ----> |    	
// | |   0    |   1   |    	
// | |        |       |	               HawaiiRG
// | |________|_______|    ---------------------------------
// | |        |       |    |       |       |       |       |
// | |        |       |    |       |       |       |       |
// | |   3    |   2   |    |       |       |       |       |
// | | -----> | ----> |    |       |       |       |       |
// |  ----------------     |       |       |       |       |
// | | -----> | ----> |    |       |       |       |       |
// | |   0    |   1   |    |   0   |   1   |   2   |   3   |
// | |        |       |    | ----> | ----> | ----> | ----> |
// | |________|_______|    ---------------------------------
// | |        |       |    	
// | |        |       |    	
// | |   3    |   2   |    	
// | | -----> | ----> |    	
// | ----------------   
// |
// |  <IN>  -> data - Pointer to the image data to deinterlace
// |  <IN>  -> cols - Number of cols in image to deinterlace
// |  <IN>  -> rows - Number of rows in image to deinterlace
// |  <IN>  -> algorithm - Algorithm number that corresponds to deint. method
// |  <IN>  -> arg - Any algorithm specific argument
// +----------------------------------------------------------------------------
void CDeinterlace::RunAlg( void *data, int rows, int cols, int algorithm, int arg )
{
	unsigned short *old_iptr = NULL;	// Old image buffer pointer
	new_iptr = NULL;					// New (temp) image buffer pointer

	// NOTE ****** Instead of memcpy old_iptr to new_iptr, maybe just return the
	// buffer pointer.

	// Form a pointer to the current image buffer
	// -------------------------------------------------------------------
	old_iptr = ( unsigned short * )data;

	// Allocate a new buffer to hold the deinterlaced image
	// -------------------------------------------------------------------
	new_iptr = new unsigned short[ cols * rows ];

	if ( new_iptr == NULL || old_iptr == NULL )
	{
		ThrowException( "RunAlg",
			"Error in allocating temporary image buffer for deinterlacing." );
	}

	switch( algorithm )
	{
		// +-------------------------------------------------------------------+
		// |                   NO DEINTERLACING                                |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_NONE:
		{
			// Do nothing
		}
		break;

		// +-------------------------------------------------------------------+
		// |                       PARALLEL READOUT                            |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_PARALLEL:
		{
			Parallel( old_iptr, rows, cols );
		}
		break;

		// +-------------------------------------------------------------------+
		// |                        SERIAL READOUT                             |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_SERIAL:
		{
			Serial( old_iptr, rows, cols );
		}
		break;

		// +-------------------------------------------------------------------+
		// |                         QUAD READOUT                              |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_CCD_QUAD:
		{
			CCDQuad( old_iptr, rows, cols );
		}
		break;

		// +-------------------------------------------------------------------+
		// |                         IR QUAD READOUT                           |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_IR_QUAD:
		{
			IRQuad( old_iptr, rows, cols );
		}
		break;

		// +-------------------------------------------------------------------+
		// |            CORRELATED DOUBLE SAMPLING IR QUAD READOUT             |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_CDS_IR_QUAD:
		{
			IRQuadCDS( old_iptr, rows, cols );
		}
		break;

		// +-------------------------------------------------------------------+
		// |         HawaiiRG READOUT                                          |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_HAWAII_RG:
		{
			HawaiiRG( old_iptr, rows, cols, arg );
		}
		break;

		// +-------------------------------------------------------------------+
		// |         STA1600 READOUT                                           |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_STA1600:
		{
			STA1600( old_iptr, rows, cols );
		}
		break;

		default:
		{
			ostringstream oss;

			oss << "Invalid deinterlace algorithm ("
			     << algorithm << ")!" << ends;

			ThrowException( "RunAlg", oss.str() );
		}
		break;
	}	// End switch

	delete[] new_iptr;
}

// +-------------------------------------------------------------------+
// | Parallel                                                          |
// +-------------------------------------------------------------------+
// |                                                                   |
// |                       Parallel                                    | 	
// |                +---------------------+                            |     	
// |                |          1  ------->|                            |  	
// |                |                     |                            |    	
// |                |                     |                            |
// |                |_____________________|                            |   	
// |                |                     |                            |   	
// |                |                     |                            |   	
// |                |                     |                            |   	
// |                |<--------  0         |                            |  	
// |                +---------------------+                            |   	
// |                                                                   |
// |  <IN>  -> rows - Number of rows in image to deinterlace           |
// |  <IN>  -> cols - Number of cols in image to deinterlace           |
// |  <IN>  -> data - Pointer to the image pixels to deinterlace       |
// +-------------------------------------------------------------------+
void CDeinterlace::Parallel( unsigned short *data, int rows, int cols )
{
	if ( ( ( float )rows/2 ) != ( int )rows/2 )
	{
		ThrowException(	"Parallel",
				"Number of rows must be EVEN for DEINTERLACE_PARALLEL." );
	}
	else
	{
		for ( int i=0; i<( cols * rows )/2; i++ )
		{
			*( new_iptr + i )                       = *( data + ( 2 * i ) );
			*( new_iptr + ( cols * rows ) - i - 1 ) = *( data + ( 2 * i ) + 1 );
		}

#ifdef WIN32
		CopyMemory( data, new_iptr, cols*rows*sizeof( unsigned short ) );
#else
		memcpy( data, new_iptr, cols*rows*sizeof( unsigned short ) );
#endif
	}
}

// +-------------------------------------------------------------------+
// | Serial                                                            |
// +-------------------------------------------------------------------+
// |                                                                   |
// |                         Serial                                    | 	
// |                +----------+----------+                            |     	
// |                |          |--------> |                            |  	
// |                |          |   1      |                            |    	
// |                |          |          |                            |
// |                |          |          |                            |
// |                |          |          |                            |
// |                |          |          |                            |
// |                |     0    |          |                            |
// |                |<-------- |          |                            |
// |                +----------+----------+                            |   	
// |                                                                   |
// |  <IN>  -> rows - Number of rows in image to deinterlace           |
// |  <IN>  -> cols - Number of cols in image to deinterlace           |
// |  <IN>  -> data - Pointer to the image pixels to deinterlace       |
// +-------------------------------------------------------------------+
void CDeinterlace::Serial( unsigned short *data, int rows, int cols )
{
	int p1    = 0;
	int p2    = 0;
	int begin = 0;
	int end   = 0;

	if ( ( float )cols/2 != ( int )cols/2 ) 
	{
		ThrowException(	"Serial",
				"Number of cols must be EVEN for DEINTERLACE_SERIAL." );
	}
	else
	{
		for ( int i=0; i<rows; i++ )
		{
			// Leave in +0 for clarity
			p1      = i * cols + 0; // Position in raw image
			p2      = i * cols + 1;
			begin   = i * cols + 0; // Position in interlaced image
			end     = i * cols + cols - 1;
       				
			for ( int j=0; j<cols; j+=2 )
			{
				*( new_iptr + begin ) = *( data + p1 );
				*( new_iptr + end )   = *( data + p2 );
				++begin;
				--end;
				p1 += 2;
				p2 += 2;
			}
		}

#ifdef WIN32
		CopyMemory( data, new_iptr, cols*rows*sizeof( unsigned short ) );
#else
		memcpy( data, new_iptr, cols*rows*sizeof( unsigned short ) );
#endif
	}
}

// +-------------------------------------------------------------------+
// | CCDQuad                                                           |
// +-------------------------------------------------------------------+
// |                                                                   |
// |                        CCD Quad                                   | 	
// |                +----------+----------+                            |     	
// |                | <--------|--------> |                            |  	
// |                |     3    |    2     |                            |    	
// |                |          |          |                            |
// |                |__________|__________|                            |
// |                |          |          |                            |
// |                |          |          |                            |
// |                |     0    |    1     |                            |    	
// |                | <--------|--------> |                            |  	
// |                +----------+----------+                            |   	
// |                                                                   |
// |  <IN>  -> rows - Number of rows in image to deinterlace           |
// |  <IN>  -> cols - Number of cols in image to deinterlace           |
// |  <IN>  -> data - Pointer to the image pixels to deinterlace       |
// +-------------------------------------------------------------------+
void CDeinterlace::CCDQuad( unsigned short *data, int rows, int cols )
{
	int i       = 0;
	int j       = 0;
	int counter = 0;
	int end     = 0;
	int begin   = 0;

	if ( ( float )cols/2 != ( int )cols/2 || ( float )rows/2 != ( int )rows/2 )
	{
		ThrowException(	"CCDQuad",
			"Number of cols AND rows must be EVEN for DEINTERLACE_CCD_QUAD." );
	}
	else
	{
		while( i < cols*rows )
		{
			if ( counter%( cols/2 ) == 0 )
			{
				end = ( cols * rows ) - ( cols * j ) - 1;
               	begin = ( cols * j ) + 0;	// Left in 0 for clarity
              	j++;						// Number of completed rows
               	counter = 0;				// Reset for next convergece
			}
        				
			*( new_iptr + begin + counter )            = *( data + i++ );	// front_row--->
			*( new_iptr + begin + cols - 1 - counter ) = *( data + i++ );	// front_row<--
			*( new_iptr + end - counter )              = *( data + i++ );	// end_row<----
			*( new_iptr + end - cols + 1 + counter )   = *( data + i++ );	// end_row---->
			counter++;
		}

#ifdef WIN32
		CopyMemory( data, new_iptr, cols*rows*sizeof( unsigned short ) );
#else
		memcpy( data, new_iptr, cols*rows*sizeof( unsigned short ) );
#endif
	}
}

// +-------------------------------------------------------------------+
// | IRQuad                                                            |
// +-------------------------------------------------------------------+
// |                                                                   |
// |                        IR Quad                                    | 	
// |                +----------+----------+                            |     	
// |                | -------> |--------> |                            |  	
// |                |    0     |    1     |                            |    	
// |                |          |          |                            |
// |                |__________|__________|                            |
// |                |          |          |                            |
// |                |          |          |                            |
// |                |    3     |    2     |                            |    	
// |                | -------> |--------> |                            |  	
// |                +----------+----------+                            |   	
// |                                                                   |
// |  <IN>  -> rows - Number of rows in image to deinterlace           |
// |  <IN>  -> cols - Number of cols in image to deinterlace           |
// |  <IN>  -> data - Pointer to the image pixels to deinterlace       |
// +-------------------------------------------------------------------+
void CDeinterlace::IRQuad( unsigned short *data, int rows, int cols )
{
	int i       = 0;
	int j       = rows - 1;
	int counter = 0;
	int end     = 0;
	int begin   = 0;

	if ( ( float )cols/2 != ( int )cols/2 || ( float )rows/2 != ( int )rows/2 )
	{
		ThrowException( "IRQuad",
			"Number of cols AND rows must be EVEN for DEINTERLACE_IR_QUAD." );
	}
	else
	{
		while( i < cols*rows )
		{
			if ( counter%( cols/2 ) == 0 )
			{
				end = ( j - ( rows/2 ) )*cols;
				begin = j*cols;
				j--; 			// Nnumber of completed rows
				counter = 0; 	// Reset for next convergece
			}

			*( new_iptr + begin + counter )              = *( data + i++ ); // front_row--->
			*( new_iptr + begin + ( cols/2 ) + counter ) = *( data + i++ ); // front_row<--
			*( new_iptr + end + ( cols/2 ) + counter )   = *( data + i++ ); // end_row<----
			*( new_iptr + end + counter )                = *( data + i++ ); // end_row---->
			counter++;
		}

#ifdef WIN32
		CopyMemory( data, new_iptr, cols*rows*sizeof( unsigned short ) );
#else
		memcpy( data, new_iptr, cols*rows*sizeof( unsigned short ) );
#endif
	}
}

// +-------------------------------------------------------------------+
// | IRQuad (Correlated Double Sampling )                              |
// +-------------------------------------------------------------------+
// |                                                                   |
// |                      IR Quad CDS                                  | 	
// |                +----------+----------+                            |     	
// |                | -------> |--------> |                            |  	
// |                |    0     |    1     |                            |    	
// |                |          |          |                            |
// |                |__________|__________|                            |
// |                |          |          |                            |
// |                |          |          |                            |
// |                |    3     |    2     |                            |    	
// |                | -------> |--------> |                            |  	
// |                +----------+----------+                            |   	
// |                | -------> |--------> |                            |  	
// |                |    0     |    1     |                            |    	
// |                |          |          |                            |
// |                |__________|__________|                            |
// |                |          |          |                            |
// |                |          |          |                            |
// |                |    3     |    2     |                            |    	
// |                | -------> |--------> |                            |  	
// |                +----------+----------+                            |   	
// |                                                                   |
// |  <IN>  -> rows - Number of rows in image to deinterlace           |
// |  <IN>  -> cols - Number of cols in image to deinterlace           |
// |  <IN>  -> data - Pointer to the image pixels to deinterlace       |
// +-------------------------------------------------------------------+
void CDeinterlace::IRQuadCDS( unsigned short *data, int rows, int cols )
{
	int i       = 0;
	int j       = 0;
	int counter = 0;
	int end     = 0;
	int begin   = 0;
	int oldRows = 0;

	unsigned short *orig_old_iptr = data;
	unsigned short *orig_new_iptr = new_iptr;

	if ( orig_old_iptr == NULL || orig_new_iptr == NULL )
	{
		ThrowException(	"IRQuadCDS",
						"One or more image buffers are NULL." );
	}

	else if ( float( cols/2 ) != int( cols/2 ) ||
			  float( rows/2 ) != int( rows/2 ) )
	{
		ThrowException(	"IRQuadCDS",
			"Number of cols AND rows must be EVEN for DEINTERLACE_CDS_IR_QUAD." );
	}

	else
	{
		// Set the the number of rows to half the image size.
		oldRows = rows;
		rows = rows/2;

		// Deinterlace the two image halves separately.
		for ( int imageSection = 0; imageSection <= 1; imageSection++ )
		{
			i = 0;
			j = rows-1;
			counter = 0;
			end = 0;
			begin = 0;

			while( i < cols*rows )
			{
       			if ( counter%( cols/2 ) == 0 )
       			{
					end = ( j - ( rows/2 ) )*cols;
					begin = j*cols;
					j--; 			// Number of completed rows
					counter = 0; 	// Reset for next convergece
				}

				*( new_iptr + begin + counter )                = *( data + i++ ); // front_row--->
				*( new_iptr + begin + ( cols / 2 ) + counter ) = *( data + i++ ); // front_row<--
				*( new_iptr + end + ( cols / 2 ) + counter )   = *( data + i++ ); // end_row<----
				*( new_iptr + end + counter )                  = *( data + i++ ); // end_row---->
				counter++;
			}
			data += rows * cols;
			new_iptr += rows * cols;
		}

		data = orig_old_iptr;
		new_iptr = orig_new_iptr;

#ifdef WIN32
		CopyMemory( data, new_iptr, cols*oldRows*sizeof( unsigned short ) );
#else
		memcpy( data, new_iptr, cols*oldRows*sizeof( unsigned short ) );
#endif
	}
}

// +-------------------------------------------------------------------+
// | HawaiiRG                                                          |
// +-------------------------------------------------------------------+
// |                                                                   |
// |                          HawaiiRG                                 | 	
// |              +-------+-------+-------+-------+                    |
// |              |       |       |       |       |                    |
// |              |       |       |       |       |                    |
// |              |       |       |       |       |                    |
// |              |       |       |       |       |                    |
// |              |       |       |       |       |                    |
// |              |       |       |       |       |                    |
// |              |   0   |   1   |   2   |   3   |                    |
// |              | ----> | ----> | ----> | ----> |                    |
// |              +-------+-------+-------+-------+                    |
// |                                                                   |
// |  <IN>  -> rows - Number of rows in image to deinterlace           |
// |  <IN>  -> cols - Number of cols in image to deinterlace           |
// |  <IN>  -> data - Pointer to the image pixels to deinterlace       |
// |  <IN>  -> arg  - The number of channels in the image (16, 32, ..) |
// +-------------------------------------------------------------------+
void CDeinterlace::HawaiiRG( unsigned short *data, int rows, int cols, int arg )
{
	const int ERR = 0x455252;

	if ( ( float )cols/2 != ( int )cols/2 )
	{
		ThrowException(	"HawaiiRG",
						"Number of cols must be EVEN for DEINTERLACE_HAWAII_RG." );
	}

	else if ( arg <= 0 || arg == ERR )
	{
		ThrowException(	"HawaiiRG",
			"The number of readout channels must be supplied for DEINTERLACE_HAWAII_RG." );
	}

	else if ( arg % 2 != 0 )
	{
		ThrowException(	"HawaiiRG",
			"The readout channel count must be EVEN for DEINTERLACE_HAWAII_RG." );
	}

	else
	{
		unsigned short *rowPtr = new_iptr;
		int offset             = cols / arg;	// Needs to be long because of array size
		int dataIndex          = 0;				// Data index

		for ( int r=0; r<rows; r++ )
		{
			rowPtr = new_iptr + ( cols * r );

			for ( int c=0; c<cols/arg; c++ )
			{
				for ( int i=0; i<arg; i++ )
					rowPtr[ c + i * offset ] = data[ dataIndex++ ];
			}
		}

#ifdef WIN32
		CopyMemory( data, new_iptr, cols*rows*sizeof( unsigned short ) );
#else
		memcpy( data, new_iptr, cols*rows*sizeof( unsigned short ) );
#endif
	}
}

// +-------------------------------------------------------------------+
// | STA1600                                                           |
// +-------------------------------------------------------------------+
// |                                                                   |
// |                            STA1600                                | 	
// |                                                                   |
// |                <-+     <-+     <-+           <-+                  |
// |              +---|---+---|---+---|---+     |---|---+              |
// |              |   |   |   |   |   |   |     |   |   |              |
// |              |   8   |   9   |   10  | ... |  15   |              |
// |              |       |       |       |     |       |              |
// |              |       |       |       |     |       |              |
// |              +-------+-------+-------+ ... +-------+              |
// |              |       |       |       |     |       |              |
// |              |       |       |       |     |       |              |
// |              |   0   |   1   |   2   | ... |   7   |              |
// |              |   |   |   |   |   |   |     |   |   |              |
// |              +---|---+---|---+---|---+     |---|---+              |
// |                  |       |       |             |                  |
// |                <-+     <-+     <-+           <-+                  |
// |                                                                   |
// |  <IN>  -> data - Pointer to the image pixels to deinterlace       |
// |  <IN>  -> rows - Number of rows in image to deinterlace           |
// |  <IN>  -> cols - Number of cols in image to deinterlace           |
// +-------------------------------------------------------------------+
void CDeinterlace::STA1600( unsigned short *data, int rows, int cols )
{
	if ( ( cols % 16 ) != 0  )
	{
		ThrowException(	"STA1600",
			"Number of cols must be a multiple of 16 for DEINTERLACE_STA1600" );
	}

	if ( ( rows % 2 ) != 0  )
	{
		ThrowException(	"STA1600",
			"Number of rows must be a multiple of 2 for DEINTERLACE_STA1600" );
	}

	else
	{
		unsigned short *botPtr = new_iptr;
		unsigned short *topPtr = new_iptr + ( cols * ( rows - 1 ) );
		long offset = cols / 8;		// Needs to be long because of array size
		long dataIndex = 0;			// Data index

		for ( long r=0; r<rows/2; r++ )
		{
			topPtr = new_iptr + ( cols * ( rows - r - 1 ) );
			botPtr = new_iptr + ( cols * r );

			for ( long c=0; c<cols/8; c++ )
			{
				botPtr[ c + 7 * offset ] = data[ dataIndex++ ];
				botPtr[ c + 6 * offset ] = data[ dataIndex++ ];
				botPtr[ c + 5 * offset ] = data[ dataIndex++ ];
				botPtr[ c + 4 * offset ] = data[ dataIndex++ ];
				botPtr[ c + 3 * offset ] = data[ dataIndex++ ];
				botPtr[ c + 2 * offset ] = data[ dataIndex++ ];
				botPtr[ c + 1 * offset ] = data[ dataIndex++ ];
				botPtr[ c + 0 * offset ] = data[ dataIndex++ ];

				topPtr[ c + 7 * offset ] = data[ dataIndex++ ];
				topPtr[ c + 6 * offset ] = data[ dataIndex++ ];
				topPtr[ c + 5 * offset ] = data[ dataIndex++ ];
				topPtr[ c + 4 * offset ] = data[ dataIndex++ ];
				topPtr[ c + 3 * offset ] = data[ dataIndex++ ];
				topPtr[ c + 2 * offset ] = data[ dataIndex++ ];
				topPtr[ c + 1 * offset ] = data[ dataIndex++ ];
				topPtr[ c + 0 * offset ] = data[ dataIndex++ ];
			}
		}

#ifdef WIN32
		CopyMemory( data, new_iptr, cols*rows*sizeof( unsigned short ) );
#else
		memcpy( data, new_iptr, cols*rows*sizeof( unsigned short ) );
#endif
	}
}

// +-------------------------------------------------------------------+
// | ThrowException                                                    |
// +-------------------------------------------------------------------+
// | Throws a std::runtime_error using the specified message string.   |
// |                                                                   |
// | <IN> -> sMethodName : The method where the exception occurred.    |
// | <IN> -> sMsg        : The error message.                          |
// +-------------------------------------------------------------------+
void CDeinterlace::ThrowException( std::string sMethodName, std::string sMsg )
{
	ostringstream oss;

	oss << "( CDeinterlace::"
		<< ( sMethodName.empty() ? "???" : sMethodName )
		<< "() ): "
		<< sMsg
		<< ends;

	throw std::runtime_error( ( const std::string )oss.str() );
}
