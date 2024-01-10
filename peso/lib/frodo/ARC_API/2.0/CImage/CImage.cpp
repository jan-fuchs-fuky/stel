#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <cstring>
#include "CImage.h"

using namespace std;
using namespace arc;

#define DEG2RAD		( 3.14159 / 180 )


//#include <fstream>
//ofstream dbgStream( "CImage_Debug.txt" );



// +----------------------------------------------------------------------------
// |  Default constructor
// +----------------------------------------------------------------------------
CImage::CImage()
{
	m_pHist = NULL;
}

// +----------------------------------------------------------------------------
// |  Default destructor
// +----------------------------------------------------------------------------
CImage::~CImage()
{
	if ( m_pHist != NULL )
		delete [] m_pHist;
}

// +----------------------------------------------------------------------------
// |  GetImageRow
// +----------------------------------------------------------------------------
// |  Returns a row of pixel data from the specified image buffer. All or part
// |  of the row can be returned.
// |
// |  <IN> -> pMem        : Pointer to image memory
// |  <IN> -> dRow        : The row to return
// |  <IN> -> dCol1       : Start column 
// |  <IN> -> dCol2       : End column
// |  <IN> -> dRows       : Number of rows in full image
// |  <IN> -> dCols       : Number of columns in full image
// |  <IN> -> dElemCount  : The number of elements in the return buffer
// |
// |  Returns: Pointer to the row data. IMPORTANT: This buffer MUST be freed
// |           by the user calling CImage::Free().
// +----------------------------------------------------------------------------
unsigned short* CImage::GetImageRow( void* pMem, int dRow, int dCol1, int dCol2,
									 int dRows, int dCols, int& dElemCount )
{
	unsigned short* pU16Buf = NULL;

	VerifyRowWithinRange( "GetImageRow", dRow,  dRows );
	VerifyColWithinRange( "GetImageRow", dCol1, dCols );
	VerifyColWithinRange( "GetImageRow", dCol2, dCols );

	VerifyRangeOrder( "GetImageRowArea", dCol1, dCol2 );

	dElemCount = ( dCol2 - dCol1 );

	if ( dElemCount < 0 )
	{
		ostringstream oss;
		oss << "Invalid column range! Column #2 [ " << dCol2
			<< " ] is less than Column #1 [ " << dCol1
			<< " ]!" << ends;

		ThrowException( "GetImageRow", oss.str() );
	}

	if ( pMem != NULL )
	{
		unsigned short* pU16Mem = static_cast<unsigned short *>( pMem );

		try
		{
			pU16Buf = new unsigned short[ size_t( dElemCount ) ];
		}
		catch ( bad_alloc& ba )
		{
			ThrowException( "GetImageRow",
							"Failed to allocate row data buffer! " +
							string( ba.what() ) );
		}

		memcpy( pU16Buf,
				&pU16Mem[ dCol1 + dRow * dCols ],
				size_t( dElemCount * sizeof( unsigned short ) ) );
	}

	return pU16Buf;
}

// +----------------------------------------------------------------------------
// |  GetImageCol
// +----------------------------------------------------------------------------
// |  Returns a column of pixel data from the specified image buffer. All or
// |  part of the column can be returned.
// |
// |  <IN> -> pMem        : Pointer to image memory
// |  <IN> -> dCol        : The column to return
// |  <IN> -> dRow1       : Start row 
// |  <IN> -> dRow2       : End row
// |  <IN> -> dRows       : Number of rows in full image
// |  <IN> -> dCols       : Number of columns in full image
// |  <IN> -> dElemCount  : The number of elements in the return buffer
// |
// |  Returns: Pointer to the column data. IMPORTANT: This buffer MUST be freed
// |           by the user calling CImage::Free().
// +----------------------------------------------------------------------------
unsigned short* CImage::GetImageCol( void* pMem, int dCol, int dRow1, int dRow2,
									 int dRows, int dCols, int& dElemCount )
{
	unsigned short* pU16Buf = NULL;

	VerifyColWithinRange( "GetImageRow", dCol,  dCols );
	VerifyRowWithinRange( "GetImageRow", dRow1, dRows );
	VerifyRowWithinRange( "GetImageRow", dRow2, dRows );

	VerifyRangeOrder( "GetImageRowArea", dRow1, dRow2 );

	dElemCount = ( dRow2 - dRow1 );

	if ( dElemCount < 0 )
	{
		ostringstream oss;
		oss << "Invalid row range! Row #2 [ " << dRow2
			<< " ] is less than Row #1 [ " << dRow1
			<< " ]!" << ends;

		ThrowException( "GetImageCol", oss.str() );
	}

	if ( pMem != NULL )
	{
		unsigned short *pU16Mem = static_cast<unsigned short *>( pMem );

		try
		{
			pU16Buf = new unsigned short[ size_t( dElemCount ) ];
		}
		catch ( bad_alloc& ba )
		{
			ThrowException(	"GetImageCol",
							"Failed to allocate column data buffer! " +
							string( ba.what() ) );
		}

		for ( int dRow=dRow1, i=0; dRow<dRow2; dRow++, i++ )
		{
			pU16Buf[ i ] = pU16Mem[ dCol + dRow * dCols ];
		}
	}

	return pU16Buf;
}

// +----------------------------------------------------------------------------
// |  GetImageRowArea
// +----------------------------------------------------------------------------
// |  Returns a row of pixel data where each value is the average over the
// |  specified range of columns.
// |
// |  <IN> -> pMem        : Pointer to image memory
// |  <IN> -> dRow1       : Start row 
// |  <IN> -> dRow2       : End row
// |  <IN> -> dCol1       : Start column 
// |  <IN> -> dCol2       : End column
// |  <IN> -> dRows       : Number of rows in full image
// |  <IN> -> dCols       : Number of columns in full image
// |  <IN> -> dElemCount  : The number of elements in the return buffer
// |
// |  Returns: Pointer to the row data. IMPORTANT: This buffer MUST be freed
// |           by the user calling CImage::Free().
// +----------------------------------------------------------------------------
float* CImage::GetImageRowArea( void* pMem, int dRow1, int dRow2, int dCol1,
							    int dCol2, int dRows, int dCols, int& dElemCount )
{
	float* pFloatBuf = NULL;
	float  fRowSum   = 0;

	VerifyRowWithinRange( "GetImageRowArea", dRow1, dRows );
	VerifyRowWithinRange( "GetImageRowArea", dRow2, dRows );
	VerifyColWithinRange( "GetImageRowArea", dCol1, dCols );
	VerifyColWithinRange( "GetImageRowArea", dCol2, dCols );

	VerifyRangeOrder( "GetImageRowArea", dCol1, dCol2 );
	VerifyRangeOrder( "GetImageRowArea", dRow1, dRow2 );

	dElemCount = ( dRow2 - dRow1 );

	if ( dElemCount < 0 )
	{
		ostringstream oss;
		oss << "Invalid row range! Row #2 [ " << dRow2
			<< " ] is less than Row #1 [ " << dRow1
			<< " ]!" << ends;

		ThrowException( "GetImageRowArea", oss.str() );
	}

	if ( pMem != NULL )
	{
		unsigned short *pU16Mem = static_cast<unsigned short *>( pMem );

		try
		{
			pFloatBuf = new float[ size_t( dElemCount ) ];
		}
		catch ( bad_alloc& ba )
		{
			ThrowException( "GetImageRowArea",
							"Failed to allocate memory for row data! " +
							string( ba.what() ) );
		}

		for ( int dRow=dRow1, i=0; dRow<dRow2; dRow++, i++ )
		{
			fRowSum = 0;

			for ( int dCol=dCol1; dCol<dCol2; dCol++ )
			{
				fRowSum += pU16Mem[ dCol + dRow * dCols ];
			}

			pFloatBuf[ i ] = fRowSum / ( ( float )( dCol2 - dCol1 ) );
		}
	}

	return pFloatBuf;
}

// +----------------------------------------------------------------------------
// |  GetImageColArea
// +----------------------------------------------------------------------------
// |  Returns a column of pixel data where each value is the average over the
// |  specified range of rows.
// |
// |  <IN> -> pMem        : Pointer to image memory
// |  <IN> -> dRow1       : Start row 
// |  <IN> -> dRow2       : End row
// |  <IN> -> dCol1       : Start column 
// |  <IN> -> dCol2       : End column
// |  <IN> -> dRows	      : Number of rows in full image
// |  <IN> -> dCols       : Number of columns in full image
// |  <IN> -> dElemCount  : The number of elements in the return buffer
// |
// |  Returns: Pointer to the column data. IMPORTANT: This buffer MUST be freed
// |           by the user calling CImage::Free().
// +----------------------------------------------------------------------------
float* CImage::GetImageColArea( void* pMem, int dRow1, int dRow2, int dCol1,
							    int dCol2, int dRows, int dCols, int& dElemCount )
{
	float* pFloatBuf = NULL;
	float  fColSum   = 0;

	VerifyRowWithinRange( "GetImageColArea", dRow1, dRows );
	VerifyRowWithinRange( "GetImageColArea", dRow2, dRows );
	VerifyColWithinRange( "GetImageColArea", dCol1, dCols );
	VerifyColWithinRange( "GetImageColArea", dCol2, dCols );

	VerifyRangeOrder( "GetImageRowArea", dCol1, dCol2 );
	VerifyRangeOrder( "GetImageRowArea", dRow1, dRow2 );

	dElemCount = ( dCol2 - dCol1 );

	if ( dElemCount < 0 )
	{
		ostringstream oss;
		oss << "Invalid column range! Column #2 [ " << dCol2
			<< " ] is less than Column #1 [ " << dCol1
			<< " ]!" << ends;

		ThrowException( "GetImageColArea", oss.str() );
	}

	if ( pMem != NULL )
	{
		unsigned short *pU16Mem = static_cast<unsigned short *>( pMem );

		try
		{
			pFloatBuf = new float[ size_t( dElemCount ) ];
		}
		catch( bad_alloc& ba )
		{
			ThrowException( "GetImageColArea",
							"Failed to allocate memory for column data! " +
							string( ba.what() ) );
		}

		for ( int dCol=dCol1, i=0; dCol<dCol2; dCol++, i++ )
		{
			fColSum = 0;

			for ( int dRow=dRow1; dRow<dRow2; dRow++ )
			{
				fColSum += pU16Mem[ dCol + dRow * dCols ];
			}

			pFloatBuf[ i ] = fColSum / ( ( float )( dRow2 - dRow1 ) );
		}
	}

	return pFloatBuf;
}

// +----------------------------------------------------------------------------
// |  Free
// +----------------------------------------------------------------------------
// |  Frees row/column buffer data returned by GetImageRow(), GetImageCol(),
// |  GetImageRowArea(), and GetImageColArea().  This method MUST be called by
// |  the user to free the returned data buffer.
// |
// |  <IN> -> pBuf        : Pointer to buffer to free
// |  <IN> -> dTypeSize   : The size of the buffer pointer data. i.e. if pBuf
// |                        points to an unsigned short buffer, then dTypeSize
// |                        should be set to sizeof( unsigned short ).
// +----------------------------------------------------------------------------
void CImage::Free( void* pBuf, int dTypeSize )
{
	if ( pBuf != NULL )
	{
		if ( dTypeSize == sizeof( unsigned short ) )
		{
			delete [] ( ( unsigned short * )pBuf );
		}

		else if ( dTypeSize == sizeof( float ) )
		{
			delete [] ( ( float * )pBuf );
		}
	}
}

// +----------------------------------------------------------------------------
// |  GetDiffStats
// +----------------------------------------------------------------------------
// |  Calculates the min, max, mean, variance, standard deviation for each image
// |  as well as the difference mean, variance and standard deviation over the
// |  specified image memory rows and cols. This is used for PTC. The two images
// |  MUST be the same size or the methods behavior is undefined as this cannot
// |  be verified using the given parameters.
// |
// |  <IN> -> pMem1 : Pointer to image #1 memory
// |  <IN> -> pMem2 : Pointer to image #2 memory
// |  <IN> -> dRow1 : Start row
// |  <IN> -> dRow2 : End row
// |  <IN> -> dCol1 : Start column
// |  <IN> -> dCol2 : End column
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in each full image
// |  <IN> -> dBpp  : The image data bits-per-pixel; either BPP16 or BPP32.
// |
// |  Returns a CImage::CImageDiffStats structure with all the statistice filled
// |  in. Throws a std::runtume_error on error.
// +----------------------------------------------------------------------------
CImage::CImgDifStats CImage::GetDiffStats( void *pMem1, void *pMem2, int dRow1,
										   int dRow2, int dCol1, int dCol2,
										   int dRows, int dCols, int dBpp )
{
	double gSum = 0.0, gDifSum = 0.0, gVal1 = 0.0, gVal2 = 0.0;

	if ( pMem1 == NULL )
	{
		ThrowException( "GetDiffStats",
						"Invalid image memory pointer parameter #1!" );
	}

	if ( pMem2 == NULL )
	{
		ThrowException( "GetDiffStats",
						"Invalid image memory pointer parameter #2!" );
	}

	VerifyRowWithinRange( "GetDiffStats", dRow1, dRows );
	VerifyRowWithinRange( "GetDiffStats", dRow2, dRows );
	VerifyColWithinRange( "GetDiffStats", dCol1, dCols );
	VerifyColWithinRange( "GetDiffStats", dCol2, dCols );

	if ( dRow1 == dRow2 ) dRow2++;
	if ( dCol1 == dCol2 ) dCol2++;
	double gTotalPixelCount = ( ( dRow2 - dRow1 ) * ( dCol2 - dCol1 ) );

	CImgDifStats cImgDifStats;
	cImgDifStats.cImg1Stats = GetStats( pMem1,
										dRow1,
										dRow2,
										dCol1,
										dCol2,
										dRows,
										dCols );

	cImgDifStats.cImg2Stats = GetStats( pMem2,
										dRow1,
										dRow2,
										dCol1,
										dCol2,
										dRows,
										dCols );

	for ( int i=dRow1; i<dRow2; i++ )
	{
		for ( int j=dCol1; j<dCol2; j++ )
		{
			gVal1 = ( double )( ( dBpp == BPP16 ) ?
				    ( ( unsigned short * )pMem1 )[ j + i * dCols ] :
				    ( ( unsigned int * )pMem1 )[ j + i * dCols ] );

			gVal2 = ( double )( ( dBpp == BPP16 ) ?
				    ( ( unsigned short * )pMem2 )[ j + i * dCols ] :
				    ( ( unsigned int * )pMem2 )[ j + i * dCols ] );

			gSum += ( gVal1 - gVal2 );

			gDifSum += ( pow( ( cImgDifStats.cImg2Stats.gMean - gVal2 ) -
					   ( cImgDifStats.cImg1Stats.gMean - gVal1 ), 2 ) );
		}
	}

	cImgDifStats.cImgDiffStats.gMean     = fabs( gSum / gTotalPixelCount );
	cImgDifStats.cImgDiffStats.gVariance = gDifSum / gTotalPixelCount;
	cImgDifStats.cImgDiffStats.gStdDev   = sqrt( cImgDifStats.cImgDiffStats.gVariance );

	return cImgDifStats;
}

// +----------------------------------------------------------------------------
// |  GetDiffStats
// +----------------------------------------------------------------------------
// |  Calculates the image min, max, mean, variance, and standard deviation over
// |  the entire image.
// |
// |  <IN> -> pMem  : Pointer to image memory
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// |  <IN> -> dBpp  : The image data bits-per-pixel; either BPP16 or BPP32.
// |
// |  Returns a CImage::CImageStats structure with all the statistice filled in.
// |  Throws a std::runtume_error on error.
// +----------------------------------------------------------------------------
CImage::CImgDifStats CImage::GetDiffStats( void *pMem1, void *pMem2, int dRows,
										   int dCols, int dBpp )
{
	return GetDiffStats( pMem1, pMem2, 0, dRows, 0, dCols, dRows, dCols, dBpp );
}

// +----------------------------------------------------------------------------
// |  GetStats
// +----------------------------------------------------------------------------
// |  Calculates the image min, max, mean, variance, and standard deviation over
// |  the specified image memory rows and cols.
// |
// |  <IN> -> pMem  : Pointer to image memory
// |  <IN> -> dRow1 : Start row
// |  <IN> -> dRow2 : End row
// |  <IN> -> dCol1 : Start column
// |  <IN> -> dCol2 : End column
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// |  <IN> -> dBpp  : The image data bits-per-pixel; either BPP16 or BPP32.
// |
// |  Returns a CImage::CImageStats structure with all the statistice filled in.
// |  Throws a std::runtume_error on error.
// +----------------------------------------------------------------------------
CImage::CImgStats CImage::GetStats( void *pMem, int dRow1, int dRow2, int dCol1,
								    int dCol2, int dRows, int dCols, int dBpp )
{
	double gVal = 0.0, gSum = 0.0, gDevSqrdSum = 0.0;
	double gMax16Bpp = T_SIZE( unsigned short );
	double gMax32Bpp = T_SIZE( unsigned int );

	if ( pMem == NULL )
	{
		ThrowException( "GetStats",
						"Invalid image memory pointer parameter!" );
	}

	VerifyRowWithinRange( "GetDiffStats", dRow1, dRows );
	VerifyRowWithinRange( "GetDiffStats", dRow2, dRows );
	VerifyColWithinRange( "GetDiffStats", dCol1, dCols );
	VerifyColWithinRange( "GetDiffStats", dCol2, dCols );

	CImgStats cImgStats;

	cImgStats.gMin = ( ( dBpp == BPP16 ) ?
						 T_SIZE( unsigned short ) :
						 T_SIZE( unsigned int ) );

	if ( dRow1 == dRow2 ) dRow2++;
	if ( dCol1 == dCol2 ) dCol2++;
	double gTotalPixelCount = ( ( dRow2 - dRow1 ) * ( dCol2 - dCol1 ) );
	cImgStats.gTotalPixels = gTotalPixelCount;

	for ( int i=dRow1; i<dRow2; i++ )
	{
		for ( int j=dCol1; j<dCol2; j++ )
		{
			gVal = ( double )( ( dBpp == BPP16 ) ?
				   ( ( unsigned short * )pMem )[ j + i * dCols ] :
				   ( ( unsigned int * )pMem )[ j + i * dCols ] );

			//
			// Determine min/max values
			//
			if ( gVal < cImgStats.gMin )
				cImgStats.gMin = gVal;

			else if ( gVal > cImgStats.gMax )
				cImgStats.gMax = gVal;

			//
			// Monitor for saturated pixels
			//
			if ( ( dBpp == BPP16 && gVal >= gMax16Bpp ) ||
				 ( dBpp == BPP32 && gVal >= gMax32Bpp ) )
			{
				cImgStats.gSaturatedPixCnt++;
			}

			gSum += gVal;
		}
	}

	// Calculate mean
	cImgStats.gMean = gSum / gTotalPixelCount;

	for ( int i=dRow1; i<dRow2; i++ )
	{
		for ( int j=dCol1; j<dCol2; j++ )
		{
			gDevSqrdSum += pow( ( double )( ( dBpp == BPP16 ) ?
							  ( ( unsigned short * )pMem )[ j + i * dCols ] :
							  ( ( unsigned int * )pMem )[ j + i * dCols ] ) - cImgStats.gMean, 2 );
		}
	}

	cImgStats.gVariance = gDevSqrdSum / gTotalPixelCount;
	cImgStats.gStdDev = sqrt( cImgStats.gVariance );

	return cImgStats;
}

// +----------------------------------------------------------------------------
// |  GetStats
// +----------------------------------------------------------------------------
// |  Calculates the image min, max, mean, variance, and standard deviation over
// |  the entire image.
// |
// |  <IN> -> pMem  : Pointer to image memory
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// |  <IN> -> dBpp  : The image data bits-per-pixel; either BPP16 or BPP32.
// |
// |  Returns a CImage::CImageStats structure with all the statistice filled in.
// |  Throws a std::runtume_error on error.
// +----------------------------------------------------------------------------
CImage::CImgStats CImage::GetStats( void *pMem, int dRows, int dCols, int dBpp )
{
	return GetStats( pMem, 0, dRows, 0, dCols, dRows, dCols, dBpp );
}

// +----------------------------------------------------------------------------
// |  Histogram
// +----------------------------------------------------------------------------
// |  Calculates the histogram over the specified image memory rows and cols.
// |
// |  <OUT> -> m_pHistSize : Number of elements in returned array ( 2^16 or 2^32 )
// |  <IN> -> pMem  : Pointer to image memory
// |  <IN> -> dRow1 : Start row
// |  <IN> -> dRow2 : End row
// |  <IN> -> dCol1 : Start column
// |  <IN> -> dCol2 : End column
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// |  <IN> -> dBpp  : The image data bits-per-pixel; either BPP16 or BPP32.
// |
// |  Returns a pointer to an array of 2^16 or 2^32 elements (depending on T
// |  parameter) that represent the histogram. This data SHOULD NOT be freed by
// |  the calling application.
// +----------------------------------------------------------------------------
int* CImage::Histogram( int& m_pHistSize, void *pMem, int dRow1, int dRow2,
					    int dCol1, int dCol2, int dRows, int dCols, int dBpp )
{
	int tSize  = int( ( ( dBpp == BPP16 ) ?
						 T_SIZE( unsigned short ) :
						 T_SIZE( unsigned int ) ) );

	if ( pMem == NULL )
	{
		ThrowException( "Histogram",
						"Invalid image memory pointer parameter!" );
	}

 	VerifyRowWithinRange( "GetDiffStats", dRow1, dRows );
	VerifyRowWithinRange( "GetDiffStats", dRow2, dRows );
	VerifyColWithinRange( "GetDiffStats", dCol1, dCols );
	VerifyColWithinRange( "GetDiffStats", dCol2, dCols );

	if ( m_pHist != NULL )
		delete [] m_pHist;

	try
	{
		m_pHist = new int[ tSize ];
	}
	catch ( bad_alloc& ba )
	{
		ThrowException( "Histogram",
						"Failed to allocate histogram array! " +
						string( ba.what() ) );
	}

	m_pHistSize = tSize;

	memset( m_pHist, 0, ( size_t )tSize * sizeof( int ) );

	if ( dRow1 == dRow2 ) dRow2++;
	if ( dCol1 == dCol2 ) dCol2++;

	for ( int i=dRow1; i<dRow2; i++ )
	{
		for ( int j=dCol1; j<dCol2; j++ )
		{
			m_pHist[ ( ( dBpp == BPP16 ) ?
				   ( ( unsigned short * )pMem )[ j + i * dCols ] :
				   ( ( unsigned int * )pMem )[ j + i * dCols ] ) ]++;
		}
	}

	return m_pHist;
}

// +----------------------------------------------------------------------------
// |  Histogram
// +----------------------------------------------------------------------------
// |  Calculates the histogram over the entire image memory.
// |
// |  <OUT> -> m_pHistSize : Number of elements in returned array ( 2^16 or 2^32 )
// |  <IN> -> pMem  : Pointer to image memory
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// |  <IN> -> dBpp  : The image data bits-per-pixel; either BPP16 or BPP32.
// |
// |  Returns a pointer to an array of 2^16 or 2^32 elements (depending on T
// |  parameter) that represent the histogram. This data SHOULD NOT be freed by
// |  the calling application.
// +----------------------------------------------------------------------------
int* CImage::Histogram( int& m_pHistSize, void *pMem, int dRows, int dCols, int dBpp )
{
	return Histogram( m_pHistSize, pMem, 0, dRows, 0, dCols, dRows, dCols, dBpp );
}

// +----------------------------------------------------------------------------
// |  FreeHistogram
// +----------------------------------------------------------------------------
// |  Frees the histogram data array.
// +----------------------------------------------------------------------------
void CImage::FreeHistogram()
{
	if ( m_pHist != NULL )
	{
		delete [] m_pHist;

		m_pHist = NULL;
	}
}

// +----------------------------------------------------------------------------
// |  Add
// +----------------------------------------------------------------------------
// |  Adds buffer 2 to buffer 1. Buffer 1 is replaced with the new data.
// |
// |  <IN> -> pMem1 : Pointer to image memory 1
// |  <IN> -> pMem1 : Pointer to image memory 2
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// +----------------------------------------------------------------------------
void CImage::Add( void* pMem1, int dRows1, int dCols1, void* pMem2, int dRows2, int dCols2 )
{
	if ( dRows1 != dRows2 )
	{
		ostringstream oss;

		oss << "Image row sizes don't match! Row1: "
			<< dRows1
			<< " Row2: "
			<< dRows2
			<< ends;

		ThrowException( "Add", oss.str() );
	}

	if ( dCols1 != dCols2 )
	{
		ostringstream oss;

		oss << "Image column sizes don't match! Col1: "
			<< dCols1
			<< " Col2: "
			<< dCols2
			<< ends;

		ThrowException( "Add", oss.str() );
	}

	if ( pMem1 == NULL )
	{
		ThrowException( "Add",
						"Invalid image memory pointer parameter #1!" );
	}

	if ( pMem2 == NULL )
	{
		ThrowException( "Add",
						"Invalid image memory pointer parameter #2!" );
	}

	unsigned short* pU16Buf1 = ( unsigned short * )pMem1;
	unsigned short* pU16Buf2 = ( unsigned short * )pMem2;

	for ( int r=0; r<dRows1; r++ )
	{
		for ( int c=0; c<dCols1; c++ )
		{
			pU16Buf1[ c + r * dCols1 ] += pU16Buf2[ c + r * dCols1 ];
		}
	}
}

// +----------------------------------------------------------------------------
// |  Subtract
// +----------------------------------------------------------------------------
// |  Subtracts buffer 2 from buffer 1. Buffer 1 is replaced with the new data.
// |
// |  <IN> -> pMem1 : Pointer to image memory 1
// |  <IN> -> pMem1 : Pointer to image memory 2
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// +----------------------------------------------------------------------------
void CImage::Subtract( void* pMem1, void* pMem2, int dRows, int dCols )
{
	unsigned short *pU16Buf1 = ( unsigned short * )pMem1;
	unsigned short *pU16Buf2 = ( unsigned short * )pMem2;

	if ( pU16Buf1 == NULL )
	{
		ThrowException( "CImage",
						"Invalid [ NULL ] image memory pointer #1!" );
	}

	if ( pU16Buf2 == NULL )
	{
		ThrowException( "CImage",
						"Invalid [ NULL ] image memory pointer #2!" );
	}

	//
	// Subtract the images. Subtract pU16Buffer1 from pU16Buffer2.
	//
	for ( int i=0; i<( dRows * dCols ); i++ )
	{
		pU16Buf1[ i ] = pU16Buf2[ i ] - pU16Buf1[ i ];
	}
}

// +----------------------------------------------------------------------------
// |  SubtractImageHalves
// +----------------------------------------------------------------------------
// |  Subtracts the first half of an image from the second half.  NOTE: The
// |  first half of the image buffer is replaced with the new image.
// |
// |  Throws std::runtime_error on if the number of rows is not even or
// |  if any of the image buffer pointers is null.
// |
// |  <IN> -> dRows - The number of rows in entire image ( includes both halves )
// |  <IN> -> dCols - The number of cols in the image
// +----------------------------------------------------------------------------
void CImage::SubtractImageHalves( void* pMem, int dRows, int dCols )
{
	if ( ( dRows % 2 ) != 0 )
	{
		ostringstream oss;

		oss << "Image must have an even number of rows [ "
			<< dRows
			<< " ]";

		ThrowException( "CImage", oss.str() );
	}

	unsigned short *pU16Buf1 = ( unsigned short * )pMem;

	unsigned short *pU16Buf2 = ( ( unsigned short * )pMem +
							   ( ( dRows * dCols ) / 2 ) );

	Subtract( pU16Buf1, pU16Buf2, ( dRows / 2 ), dCols );
}

// +----------------------------------------------------------------------------
// |  Divide
// +----------------------------------------------------------------------------
// |  Divides buffer 1 by buffer 2. Buffer 1 is replaced with the new data.
// |
// |  <IN> -> pMem1 : Pointer to image memory 1
// |  <IN> -> pMem1 : Pointer to image memory 2
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// +----------------------------------------------------------------------------
void CImage::Divide( void* pMem1, void* pMem2, int dRows, int dCols )
{
}

// +----------------------------------------------------------------------------
// |  GradientFill
// +----------------------------------------------------------------------------
// |  Fills the image memory with a gradient pattern.
// |
// |  <IN> -> pMem  : Pointer to image memory
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// +----------------------------------------------------------------------------
void CImage::GradientFill( void *pMem, int dRows, int dCols )
{
	if ( pMem == NULL )
	{
		ThrowException( "GradientFill",
						"Invalid image memory pointer parameter!" );
	}

	memset( pMem, 0, ( dRows * dCols * sizeof( unsigned short ) ) );

	unsigned short* pU16Buf = ( unsigned short * )pMem;
	unsigned short  val     = 0;

	for ( int r=0; r<dRows; r++ )
	{
		for ( int c=0; c<dCols; c++ )
		{
			pU16Buf[ c + r * dCols ] = val;
		}
		val += ( 2^16 ) / dRows;
	}
}

// +----------------------------------------------------------------------------
// |  SmileyFill
// +----------------------------------------------------------------------------
// |  Fills the image memory with zeroes and puts a smiley face at the center.
// |
// |  <IN> -> pMem  : Pointer to image memory
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// +----------------------------------------------------------------------------
void CImage::SmileyFill( void *pMem, int dRows, int dCols )
{
	if ( pMem == NULL )
	{
		ThrowException( "SmileyFill",
						"Invalid image memory pointer parameter!" );
	}

	memset( pMem, 0, ( dRows * dCols * sizeof( unsigned short ) ) );

	int radius = min( ( dRows / 2 ), ( dCols / 2 ) ) - 10;

	//  Draw Head
	// +---------------------------------------------------------------------------- +
	DrawGradientFillCircle( ( dCols / 2 ),
							( dRows / 2 ),
							radius,
							dRows,
							dCols,
							pMem );

	//  Draw Left Eye
	// +---------------------------------------------------------------------------- +
	int rowFactor = int( radius / 2.5 );

	DrawFillCircle( ( dCols / 2 ) - rowFactor,
					( dRows / 2 ) + rowFactor,
					( radius / 5 ),
					dCols,
					pMem );

	//  Draw Right Eye
	// +---------------------------------------------------------------------------- +
	DrawFillCircle( ( dCols / 2 ) + rowFactor,
					( dRows / 2 ) + rowFactor,
					( radius / 5 ),
					dCols,
					pMem );

	//  Draw Mouth
	// +---------------------------------------------------------------------------- +
	for ( int i=0; i<( radius / 2 ); i++ )
	{
		DrawSemiCircle( ( dCols / 2 ),
						( dRows / 2 ) - ( rowFactor / 2 ),
						i,
						dCols,
						pMem );
	}
}

// +----------------------------------------------------------------------------
// |  VerifyImageAsSynthetic
// +----------------------------------------------------------------------------
// |  Verifies that the kernel buffer contains a valid synthetic image.
// |
// |  NOTE: The PCI DSP firmware has an issue where img[ 0 ] = 1 up to 65535,
// |  then, img[ 65536 ] = 0, where it should be 1. So this method takes this
// |  into account by reading and setting compareValue to the first pixel for
// |  each new 65535 pixel segment.
// |
// |  Throws: std::runtime_error on error
// |
// |  <IN> -> dRows  - The number of rows in the synthetic image.
// |  <IN> -> dCols  - The number of columns in the synthetic image.
// +----------------------------------------------------------------------------
void CImage::VerifyImageAsSynthetic( void* pMem, int dRows, int dCols )
{
	unsigned short* pU16Buf = ( unsigned short * )pMem;
	unsigned int udCompareValue = 0;

	if ( pU16Buf == NULL )
	{
		ThrowException( "CImage",
						"Invalid image memory pointer parameter!" );
	}

	if ( dRows <= 0 )
	{
		ostringstream oss;
		oss << "Invalid row value -> " << dRows << ends;

		ThrowException( "CImage", oss.str() );
	}

	if ( dCols <= 0 )
	{
		ostringstream oss;
		oss << "Invalid column value -> " << dCols << ends;

		ThrowException( "CImage", oss.str() );
	}

	udCompareValue = pU16Buf[ 0 ];

	for ( int row=0; row<dRows; row++ )
	{
		for ( int col=0; col<dCols; col++ )
		{
			unsigned short u16Val = pU16Buf[ col + row * dCols ];

			if ( u16Val != udCompareValue )
			{
				ostringstream oss;

				oss << "Synthetic image check failed at: " << endl
					<< "row: " << ( row + 1 )
					<< "  col: " << ( col + 1 ) << " ( mem[ "
					<< ( col + row * dCols ) << " ] )" << endl
					<< "Found value: " << u16Val << " Expected value: "
					<< udCompareValue << ends;

				ThrowException( "CImage", oss.str() );
			}
			udCompareValue++;

			if ( udCompareValue > 65535 )
			{
				udCompareValue = pU16Buf[ col + row * dCols + 1 ];
			}
		}
	}
}

// +----------------------------------------------------------------------------
// |  DrawCircle
// +----------------------------------------------------------------------------
// |  Draws a circle on the specified memory buffer.
// |
// |  <IN> -> xCenter : x position of circle center point
// |  <IN> -> yCenter : y position of circle center point
// |  <IN> -> radius  : The radius of the circle
// |  <IN> -> dCols   : Number of columns in full image
// |  <IN> -> pMem    : Pointer to image memory
// |  <IN> -> dColor  : Color to draw circle ( default = 0 )
// +----------------------------------------------------------------------------
void CImage::DrawCircle( int xCenter, int yCenter, int radius, int dCols,
						 void* pMem, int dColor )
{
	if ( pMem == ( void * )0 )
	{
		ThrowException( "DrawCircle",
						"Invalid image memory pointer parameter!" );
	}

	unsigned short* pU16Buf = ( unsigned short * )pMem;

	for ( double angle=0; angle<360; angle+=0.1 )
	{
		int x = int( radius * cos( angle * DEG2RAD ) + xCenter );
		int y = int( radius * sin( angle * DEG2RAD ) + yCenter );
		pU16Buf[ x + y * dCols ] = dColor;
	}
}

// +----------------------------------------------------------------------------
// |  DrawFillCircle
// +----------------------------------------------------------------------------
// |  Draws a filled circle on the specified memory buffer.
// |
// |  <IN> -> xCenter : x position of circle center point
// |  <IN> -> yCenter : y position of circle center point
// |  <IN> -> radius  : The radius of the circle
// |  <IN> -> dCols   : Number of columns in full image
// |  <IN> -> pMem    : Pointer to image memory
// |  <IN> -> dColor  : Color to draw circle ( default = 0 )
// +----------------------------------------------------------------------------
void CImage::DrawFillCircle( int xCenter, int yCenter, int radius, int dCols,
							 void* pMem, int dColor )
{
	if ( pMem == ( void * )0 )
	{
		ThrowException( "DrawCircle",
						"Invalid image memory pointer parameter!" );
	}

	unsigned short* pU16Buf = ( unsigned short * )pMem;

	for ( int r=0; r<radius; r++ )
	{
		for ( double angle=0; angle<360; angle+=0.1 )
		{
			int x = int( r * cos( angle * DEG2RAD ) + xCenter );
			int y = int( r * sin( angle * DEG2RAD ) + yCenter );
			pU16Buf[ x + y * dCols ] = dColor;
		}
	}
}

// +----------------------------------------------------------------------------
// |  DrawGradientFillCircle
// +----------------------------------------------------------------------------
// |  Draws a gradient filled circle on the specified memory buffer.
// |
// |  <IN> -> xCenter : x position of circle center point
// |  <IN> -> yCenter : y position of circle center point
// |  <IN> -> radius  : The radius of the circle
// |  <IN> -> dCols   : Number of columns in full image
// |  <IN> -> pMem    : Pointer to image memory
// +----------------------------------------------------------------------------
void CImage::DrawGradientFillCircle( int xCenter, int yCenter, int radius,
									 int dRows, int dCols, void* pMem )
{
	if ( pMem == ( void * )0 )
	{
		ThrowException( "DrawCircle",
						"Invalid image memory pointer parameter!" );
	}

	for ( int r=0; r<radius; r++ )
	{
		DrawCircle( ( dCols / 2 ),
					( dRows / 2 ),
					( radius - r ),
					 dCols,
					 pMem,
					 r + ( ( ( 2^16 ) - 1 ) / radius ) );
	}
}

// +----------------------------------------------------------------------------
// |  DrawSemiCircle
// +----------------------------------------------------------------------------
// |  Draws a semi-circle on the specified memory buffer.
// |
// |  <IN> -> xCenter : x position of semi-circle center point
// |  <IN> -> yCenter : y position of semi-circle center point
// |  <IN> -> radius  : The radius of the semi-circle
// |  <IN> -> dCols   : Number of columns in full image
// |  <IN> -> pMem    : Pointer to image memory
// |  <IN> -> dColor  : Color to draw semi-circle ( default = 0 )
// +----------------------------------------------------------------------------
void CImage::DrawSemiCircle( int xCenter, int yCenter, int radius, int dCols,
							 void* pMem, int dColor )
{
	if ( pMem == ( void * )0 )
	{
		ThrowException( "DrawSemiCircle",
						"Invalid image memory pointer parameter!" );
	}

	unsigned short* pU16Buf = ( unsigned short * )pMem;

	for ( double angle=180; angle<360; angle+=0.1 )
	{
		int x = int( radius * cos( angle * DEG2RAD ) + xCenter );
		int y = int( radius * sin( angle * DEG2RAD ) + yCenter );
		pU16Buf[ x + y * dCols ] = dColor;
	}
}

// +----------------------------------------------------------------------------
// |  VerifyRangeOrder
// +----------------------------------------------------------------------------
// |  Throws a runtime_error exception if value 2 is less than value 1.  This
// |  is used to ensure the coordinate range is valid.
// |
// |  <IN> -> sMethodName : Method where the error occurred
// |  <IN> -> dValue1     : The first ( lesser ) range value
// |  <IN> -> dValue2     : The second ( higher ) range value
// +----------------------------------------------------------------------------
void CImage::VerifyRangeOrder( string sMethodName, int dValue1, int dValue2 )
{
	if ( dValue2 < dValue1 )
	{
		ostringstream oss;

		oss << "Invalid range order [ " << dValue2 << " < "
			<< dValue1 << " ]! Values must be reversed!"
			<< ends;

		ThrowException( sMethodName, oss.str() );
	}
}

// +----------------------------------------------------------------------------
// |  VerifyRowWithinRange
// +----------------------------------------------------------------------------
// |  Throws a runtime_error exception if the specified row is less than zero
// |  or greater than the specified image row length.
// |
// |  <IN> -> sMethodName : Method where the error occurred
// |  <IN> -> dRow        : The row to check
// |  <IN> -> dRows       : The total row length ( i.e. image row count )
// +----------------------------------------------------------------------------
void CImage::VerifyRowWithinRange( string sMethodName, int dRow, int dRows )
{
	if ( ( dRow < 0 ) || ( dRow > dRows ) )
	{
		ostringstream oss;

		oss << "Invalid row [ " << dRow << " ]! Must be between 0 and "
			<< dRows << "!" << ends;

		ThrowException( sMethodName, oss.str() );
	}
}

// +----------------------------------------------------------------------------
// |  VerifyColWithinRange
// +----------------------------------------------------------------------------
// |  Throws a runtime_error exception if the specified column is less than
// |  zero or greater than the specified image column length.
// |
// |  <IN> -> sMethodName : Method where the error occurred
// |  <IN> -> dCol        : The column to check
// |  <IN> -> dCols       : The total column length ( i.e. image column count )
// +----------------------------------------------------------------------------
void CImage::VerifyColWithinRange( string sMethodName, int dCol, int dCols )
{
	if ( ( dCol < 0 ) || ( dCol > dCols ) )
	{
		ostringstream oss;

		oss << "Invalid column [ " << dCol << " ]! Must be between 0 and "
			<< dCols << "!" << ends;

		ThrowException( sMethodName, oss.str() );
	}
}

// +----------------------------------------------------------------------------
// |  ThrowException
// +----------------------------------------------------------------------------
// |  Method to throw a "const char *" exception.
// |
// |  <IN> -> sMethodName : The method where the exception occurred.
// |  <IN> -> sMsg        : A std::string to use for the exception message.
// +----------------------------------------------------------------------------
void CImage::ThrowException( std::string sMethodName, std::string sMsg )
{
	ostringstream oss;

	oss << "( CImage::"
		<< ( sMethodName.empty() ? "???" : sMethodName )
		<< "() ): "
		<< sMsg
		<< ends;

	throw std::runtime_error( ( const string )oss.str() );
}
