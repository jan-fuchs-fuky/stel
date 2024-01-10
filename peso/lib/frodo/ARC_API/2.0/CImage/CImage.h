#ifndef _ARC_CIMAGE_H_
#define _ARC_CIMAGE_H_

#ifdef WIN32
#include <windows.h>
#endif

#include <string>
#include <cmath>
#include "DllMain.h"


namespace arc
{
	#define T_SIZE( t )	pow( ( double )2, ( double )( sizeof( t ) * 8 ) )

	// This class is exported from the DllTest.dll
	class DLLARCIMAGE_API CImage
	{
	public:
		// Default constructor/destructor
		// ------------------------------------------------------------------------
		CImage();
		~CImage();

		// Image data
		// ------------------------------------------------------------------------
		unsigned short* GetImageRow( void* pMem, int dRow, int dCol1, int dCol2, int dRows, int dCols, int& dElemCount );
		unsigned short* GetImageCol( void* pMem, int dCol, int dRow1, int dRow2, int dRows, int dCols, int& dElemCount );

		float* GetImageRowArea( void* pMem, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int& dElemCount );
		float* GetImageColArea( void* pMem, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int& dElemCount );

		void Free( void* pBuf, int dTypeSize = sizeof( unsigned short ) );

		// Statistics
		// ------------------------------------------------------------------------
		class CImgStats
		{
		public:
			CImgStats()
			{
				gMin = gMax = gMean = gVariance = gStdDev = gTotalPixels = gSaturatedPixCnt = 0;
			}

			double gTotalPixels;
			double gMin;
			double gMax;
			double gMean;
			double gVariance;
			double gStdDev;
			double gSaturatedPixCnt;
		};

		class CImgDifStats
		{
		public:
			CImgStats cImg1Stats;
			CImgStats cImg2Stats;
			CImgStats cImgDiffStats;
		};

		CImgDifStats GetDiffStats( void *pMem1, void *pMem2, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int dBpp = BPP16 );
		CImgDifStats GetDiffStats( void *pMem1, void *pMem2, int dRows, int dCols, int dBpp = BPP16 );

		CImgStats GetStats( void *pMem, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int dBpp = BPP16 );
		CImgStats GetStats( void *pMem, int dRows, int dCols, int dBpp = BPP16 );

		// Histogram
		// ------------------------------------------------------------------------
		int* Histogram( int& pHistSize, void *pMem, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int dBpp = BPP16 );
		int* Histogram( int& pHistSize, void *pMem, int dRows, int dCols, int dBpp = BPP16 );
		void FreeHistogram();

		// Image Buffer Manipulation
		// ------------------------------------------------------------------------
		void Add( void* pMem1, int dRows1, int dCols1, void* pMem2, int dRows2, int dCols2 );
		void Subtract( void* pMem1, void* pMem2, int dRows, int dCols );
		void SubtractImageHalves( void* pMem, int dRows, int dCols );
		void Divide( void* pMem1, void* pMem2, int dRows, int dCols );

		// Image Buffer Fill
		// ------------------------------------------------------------------------
		void GradientFill( void* pMem, int dRows, int dCols );
		void SmileyFill( void *pMem, int dRows, int dCols );

		// Synthetic Image
		// ------------------------------------------------------------------------
		void VerifyImageAsSynthetic( void* pMem, int dRows, int dCols );

		// Constants
		// ------------------------------------------------------------------------
		const static int BPP16 = 16;
		const static int BPP32 = 32;

		const static int FLOAT_TYPE = sizeof( float );
		const static int U16_TYPE   = sizeof( unsigned short );

	private:
		void DrawSemiCircle( int xCenter, int yCenter, int radius, int dCols, void* pMem, int dColor = 0 );
		void DrawFillCircle( int xCenter, int yCenter, int radius, int dCols, void* pMem, int dColor = 0 );
		void DrawGradientFillCircle( int xCenter, int yCenter, int radius, int dRows, int dCols, void* pMem );
		void DrawCircle( int xCenter, int yCenter, int radius, int dCols, void* pMem, int dColor = 0 );
		void ThrowException( std::string sMethodName, std::string sMsg );
		void VerifyRangeOrder( std::string sMethodName, int dValue1, int dValue2 );
		void VerifyRowWithinRange( std::string sMethodName, int dRow, int dRows );
		void VerifyColWithinRange( std::string sMethodName, int dCol, int dCols );

		int* m_pHist;
	};

}	// end namespace

#endif
