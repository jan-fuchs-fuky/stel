#ifndef _ARC_CFITS_FILE_H_
#define _ARC_CFITS_FILE_H_

#include <stdexcept>
#include <string>
#include "fitsio.h"
#include "DllMain.h"


namespace arc
{
	#define T_SIZE( t )	pow( ( double )2, ( double )( sizeof( t ) * 8 ) )


	// This class is exported from the DllTest.dll
	class DLLFITSFILE_API CFitsFile
	{
	public:
		// Default constructor/destructor
		// ------------------------------------------------------------------------
//		 CFitsFile();
		~CFitsFile();

		// Read/Write existing file constructor
		// ------------------------------------------------------------------------
		CFitsFile( const char* pszFilename, int dMode = READMODE );

		// Create new file constructor
		// ------------------------------------------------------------------------
		CFitsFile( const char* pszFilename, int dRows, int dCols, int dBitsPerPixel = BPP16, bool bIs3D = false );

		//void Assign( const char* pszFilename, int dMode = READMODE );
		//void Assign( const char* pszFilename, int dRows, int dCols, int dBitsPerPixel = BPP16, bool bIs3D = false );

		//void Open( const char* pszFilename, int dMode = READMODE );
		//void Open( const char* pszFilename, int dRows, int dCols, int dBitsPerPixel = BPP16, bool bIs3D = false );
		//void Close();

		// General methods
		// ------------------------------------------------------------------------
		const std::string GetFilename();
		char** GetHeader( int* pKeyCount );
		void WriteKeyword( char* szKey, void* pKeyVal, int dValType, char* szComment );
		void UpdateKeyword( char* szKey, void* pKeyVal, int dValType, char* szComment );
		void GetParameters( long* pNaxes, int* pNaxis = NULL, int* pBitsPerPixel = NULL );
		void GenerateTestData();
		void ReOpen();

		// Single image methods
		// ------------------------------------------------------------------------
		bool Compare( CFitsFile& anotherCFitsFile );
		void Resize( int dRows, int dCols );
		void Write( void* pData );
		void Write( void* pData, unsigned int bytesToWrite, int fPixl = -1 );
		void WriteSubImage( int llrow, int llcol, int urrow, int urcol, void* pData );
		void *ReadSubImage( int llrow, int llcol, int urrow, int urcol );
		void *Read();

		// Data cube methods
		// ------------------------------------------------------------------------
		void Write3D( void* pData );
		void ReWrite3D( void* pData, int dImageNumber );
		void *Read3D( int dImageNumber );

		// Constants
		// ------------------------------------------------------------------------
		const static int READMODE       = READONLY;
		const static int READWRITEMODE  = READWRITE;

		const static int BPP16          = 16;
		const static int BPP32          = 32;

		const static int NAXES_COL		= 0;
		const static int NAXES_ROW		= 1;
		const static int NAXES_NOF		= 2;
		const static int NAXES_SIZE		= 3;

		const static int FITS_STRING_KEY	= 0;
		const static int FITS_INTEGER_KEY	= 1;
		const static int FITS_DOUBLE_KEY	= 2;
		const static int FITS_LOGICAL_KEY	= 3;
		const static int FITS_COMMENT_KEY	= 4;
		const static int FITS_HISTORY_KEY	= 5;
		const static int FITS_DATE_KEY		= 6;

	private:
		void ThrowException( std::string sMethodName, int dFitsStatus );
		void ThrowException( std::string sMethodName, std::string sMsg );
		void DeleteBuffer();

		fitsfile* m_fptr;
		char**    m_szDataHeader;
		void*     m_pDataBuffer;
		int       m_dFPixel;
		long      m_lFrame;
	};

}	// end namespace

#endif
