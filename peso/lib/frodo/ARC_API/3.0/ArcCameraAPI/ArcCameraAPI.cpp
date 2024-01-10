// ArcCAPI.cpp : Defines the exported functions for the DLL application.
//

#include "DllMain.h"
#include "ArcCameraAPI.h"
#include "CDeinterlace.h"
#include "CArcDevice.h"
#include "CFitsFile.h"
#include "CArcTools.h"
#include "CArcPCIe.h"
#include "CArcPCI.h"
#include "CImage.h"

#include <iostream>
#include <sstream>

using namespace std;
using namespace arc;


// +------------------------------------------------------------------------------------+
// | Define system dependent macros
// +------------------------------------------------------------------------------------+
#ifdef WIN32
#define ArcSprintf( dst, size, fmt, msg )	\
				sprintf_s( dst, fmt, msg )
#else
#define ArcSprintf( dst, size, fmt, msg )	\
				sprintf( dst, fmt, msg )
#endif


// +------------------------------------------------------------------------------------+
// | Define macros to clear/set status structure
// +------------------------------------------------------------------------------------+
#define CLEAR_STATUS( s ) \
				if ( s != ( struct ArcCAPIStatus * )0 ) {					\
				s->dSuccess = 0;											\
				s->szMessage[ 0 ] = '\0'; }

#define STATUS_OK( s ) \
				if ( s != ( struct ArcCAPIStatus * )0 ) {					\
				s->dSuccess = 1;											\
				s->szMessage[ 0 ] = '\0'; }

#define STATUS_ERR( s, msg ) \
				if ( s != ( struct ArcCAPIStatus * )0 ) {					\
				s->dSuccess = 0;											\
				ArcSprintf( s->szMessage, STATUS_MSG_SIZE, "%s", msg ); }


// +------------------------------------------------------------------------------------+
// | Define repetitive try/catch blocks
// +------------------------------------------------------------------------------------+
#define TRY_CATCH( x, s ) \
				CLEAR_STATUS( s )												\
				try { x; STATUS_OK( s ) }										\
				catch ( std::exception &e ) { STATUS_ERR( s, e.what() )	}		\

#define TRY_CATCH_PTR( x, s ) \
				CLEAR_STATUS( s )												\
				void* ptr = ( void * )0;										\
				try { ptr = x; STATUS_OK( s ) }									\
				catch ( std::exception &e ) { STATUS_ERR( s, e.what() ) }		\
				return ptr;

#define TRY_CATCH_INTPTR( x, s ) \
				CLEAR_STATUS( s )												\
				int* pInt = ( int * )0;											\
				try { pInt = x; STATUS_OK( s ) }								\
				catch ( std::exception &e ) { STATUS_ERR( s, e.what() ) }		\
				return pInt;

#define TRY_CATCH_INT( x, s ) \
				CLEAR_STATUS( s )												\
				int dInt = 0;													\
				try { dInt = x; STATUS_OK( s ) }								\
				catch ( std::exception &e ) { STATUS_ERR( s, e.what() ) }		\
				return dInt;

#define TRY_CATCH_ULONG( x, s ) \
				CLEAR_STATUS( s )												\
				unsigned long ulVal = 0;										\
				try { ulVal = x; STATUS_OK( s ) }								\
				catch ( std::exception &e ) { STATUS_ERR( s, e.what() ) }		\
				return ulVal;

#define TRY_CATCH_BOOL( x, s ) \
				CLEAR_STATUS( s )												\
				int dInt = 0;													\
				try { dInt = static_cast<int>( x ); STATUS_OK( s ) }			\
				catch ( std::exception &e ) { STATUS_ERR( s, e.what() ) }		\
				return dInt;

#define TRY_CATCH_DOUBLE( x, s ) \
				CLEAR_STATUS( s )												\
				double gDouble = 0.0;											\
				try { gDouble = x; STATUS_OK( s ) }								\
				catch ( std::exception &e ) { STATUS_ERR( s, e.what() ) }		\
				return gDouble;


// +------------------------------------------------------------------------------------+
// | Define class pointer macro
// +------------------------------------------------------------------------------------+
#define VERIFY_CLASS_PTR( clzPtr, ptr, s ) \
				if ( ptr == NULL ) {								\
					if ( s != ( struct ArcCAPIStatus * )0 ) {		\
					s->dSuccess = 0;								\
					ArcSprintf( s->szMessage, STATUS_MSG_SIZE,		\
								"( %s ): Invalid class pointer!",	\
								clzPtr ); } }


// +------------------------------------------------------------------------------------+
// | Deinterlace constants
// +------------------------------------------------------------------------------------+
const int DEINTERLACE_NONE        = CDeinterlace::DEINTERLACE_NONE;
const int DEINTERLACE_PARALLEL    = CDeinterlace::DEINTERLACE_PARALLEL;
const int DEINTERLACE_SERIAL      = CDeinterlace::DEINTERLACE_SERIAL;
const int DEINTERLACE_CCD_QUAD    = CDeinterlace::DEINTERLACE_CCD_QUAD;
const int DEINTERLACE_IR_QUAD     = CDeinterlace::DEINTERLACE_IR_QUAD;
const int DEINTERLACE_CDS_IR_QUAD = CDeinterlace::DEINTERLACE_CDS_IR_QUAD;
const int DEINTERLACE_HAWAII_RG   = CDeinterlace::DEINTERLACE_HAWAII_RG;
const int DEINTERLACE_STA1600	  = CDeinterlace::DEINTERLACE_STA1600;


// +------------------------------------------------------------------------------------+
// | Create exposure interface
// +------------------------------------------------------------------------------------+
class IFExpose :  public CExpIFace
{
public:
	IFExpose( void ( *pExposeCall )( float ) = NULL, void ( *pReadCall )( int ) = NULL )
	{
		pECFunc = pExposeCall;
		pRCFunc = pReadCall;
	}

	void ExposeCallback( float fElapsedTime )
	{
		( *pECFunc )( fElapsedTime );
	}

	void ReadCallback( int dPixelCount )
	{
		( *pRCFunc )( dPixelCount );
	}

	void ( *pECFunc )( float );
	void ( *pRCFunc )( int );
};


// +------------------------------------------------------------------------------------+
// | Create continuous exposure interface
// +------------------------------------------------------------------------------------+
class IFConExp :  public CConIFace
{
public:
	IFConExp( void ( *pFrameCall )( int, int, int, int, void* ) = NULL )
	{
		pFCFunc = pFrameCall;
	}

	void FrameCallback( int   dFPBCount,
						int   dPCIFrameCount,
						int   dRows,
						int   dCols,
						void* pBuffer )
	{
		( *pFCFunc )( dFPBCount, dPCIFrameCount, dRows, dCols, pBuffer );
	}

	void ( *pFCFunc )( int, int, int, int, void* );
};


// +------------------------------------------------------------------------------------+
// | Create CArcDevice object
// +------------------------------------------------------------------------------------+
static auto_ptr<CArcDevice> pCArcDev( new CArcPCIe() );
static auto_ptr<CDeinterlace> pCDLacer( new CDeinterlace() );
static auto_ptr<CImage> pCImage( new CImage() );
static auto_ptr<char*> pszDevList( NULL );


// +------------------------------------------------------------------------------------+
// | CArcDevice C interface functions
// +------------------------------------------------------------------------------------+

ARCCAM_API void
ArcCam_GetDeviceList( struct ArcCAPIDevList* pDevList, struct ArcCAPIStatus* pStatus )
{
	char**       pszPCIDevList	= NULL;
	char**       pszPCIeDevList	= NULL;
	CArcPCIe     cArcPCIe;
	CArcPCI		 cArcPCI;

	try
	{
		//
		//  Get the PCI device list
		// -------------------------------------------------
		CArcPCI::FindDevices();
			
		//
		// Get the PCI device string list. Ignore NULL list,
		// will handle down below.
		//
		pszPCIDevList = ( char ** )CArcPCI::GetDeviceStringList();
	}
	catch ( ... ) {}
		
	try
	{
		//
		//  Get the PCIe device list
		// -------------------------------------------------
		CArcPCIe::FindDevices();
			
		//
		// Get the PCIe device string list. Ignore NULL list,
		// will handle down below.
		//
		pszPCIeDevList = ( char ** )CArcPCIe::GetDeviceStringList();
	}
	catch ( ... ) {}
		
	try
	{
		if ( ( pszPCIDevList == NULL ) && ( pszPCIeDevList == NULL ) )
		{
			throw runtime_error(
					"( ArcCam_GetDeviceList ): Failed to retrieve device string list!" );
		}

		//
		// Get the available device count
		//
		if ( pDevList == NULL )
		{
			throw runtime_error(
					"( ArcCam_GetDeviceList ): Device list parameter cannot be NULL!" );
		}

		pDevList->dDevCount = CArcPCI::DeviceCount() + CArcPCIe::DeviceCount();

		if ( pDevList->dDevCount > 0 )
		{
			ostringstream oss;
					
			//
			// Add the PCIe devices
			//
			for ( int i=0; i<CArcPCIe::DeviceCount(); i++ )
			{
				ArcSprintf( pDevList->szDevList[ i ],
						   DEV_NAME_SIZE,
						   "%s",
						   pszPCIeDevList[ i ] );
			}

			//
			// Add the PCI devices
			//
			for ( int i=0, j=CArcPCIe::DeviceCount(); i<CArcPCI::DeviceCount(); i++, j++ )
			{
				ArcSprintf( pDevList->szDevList[ j ],
						   DEV_NAME_SIZE,
						   "%s",
						   pszPCIDevList[ i ] );
			}
		}
		else
		{
			cArcPCIe.FreeDeviceStringList();
			cArcPCI.FreeDeviceStringList();

			throw runtime_error(
					"( ArcCam_GetDeviceList ): Failed create array for device list!" );
		}

		cArcPCIe.FreeDeviceStringList();
		cArcPCI.FreeDeviceStringList();
	}
	catch ( std::exception &e )
	{
		cArcPCIe.FreeDeviceStringList();
		cArcPCI.FreeDeviceStringList();

		STATUS_ERR( pStatus, e.what() );
	}
	catch ( ... )
	{
		cArcPCIe.FreeDeviceStringList();
		cArcPCI.FreeDeviceStringList();

		STATUS_ERR( pStatus, "( ArcCam_GetDeviceList ): Unknown error!" );
	}
}


ARCCAM_API int
ArcCam_IsOpen( struct ArcCAPIStatus* pStatus )
{
	TRY_CATCH_BOOL( pCArcDev.get()->IsOpen(), pStatus )
}


ARCCAM_API void
ArcCam_Open( int dDevNum, struct ArcCAPIStatus* pStatus )
{
	try
	{
		pCArcDev.get()->Open( dDevNum );
	}
	catch ( std::exception &e )
	{
		STATUS_ERR( pStatus, e.what() );
	}
	catch ( ... )
	{
		STATUS_ERR( pStatus, "( ArcCam_Open ): Unknown error!" );
	}
}


ARCCAM_API void
ArcCam_OpenWithBuffer( int dDevNum, int dBytes, struct ArcCAPIStatus* pStatus )
{
	ArcCam_Open( dDevNum, pStatus );

	if ( pStatus->dSuccess )
	{
		ArcCam_MapCommonBuffer( dBytes, pStatus );
	}
}


ARCCAM_API void
ArcCam_OpenByName( const char* pszDeviceName, struct ArcCAPIStatus* pStatus )
{
	if ( pszDeviceName != NULL )
	{
		try
		{
			CArcTools::CTokenizer cTokenizer;
			cTokenizer.Victim( pszDeviceName );

			string sDevice = cTokenizer.Next();

			if ( sDevice.compare( "PCI" ) == 0 )
			{
				CArcPCI::FindDevices();

				pCArcDev.reset( new CArcPCI() );
			}

			else if ( sDevice.compare( "PCIe" ) == 0 )
			{
				CArcPCIe::FindDevices();
						
				pCArcDev.reset( new CArcPCIe() );
			}

			else
			{
				throw runtime_error(
							"( ArcCam_OpenByName ): Invalid device: " + sDevice );
			}

			cTokenizer.Next();		// Dump "Device" Text

			int dDeviceNumber = atoi( cTokenizer.Next().c_str() );

			pCArcDev.get()->Open( dDeviceNumber );
		}
		catch ( std::exception &e )
		{
			STATUS_ERR( pStatus, e.what() );
		}
		catch ( ... )
		{
			STATUS_ERR( pStatus, "( ArcCam_OpenByName ): Unknown error!" );
		}
	}
	else
	{
		STATUS_ERR(
				 pStatus,
				"( ArcCam_OpenByName ): Device name parameter cannot be NULL!" );
	}
}


ARCCAM_API void
ArcCam_OpenByNameWithBuffer( const char* pszDeviceName, int dBytes, struct ArcCAPIStatus* pStatus )
{
	ArcCam_OpenByName( pszDeviceName, pStatus );

	if ( pStatus->dSuccess )
	{
		ArcCam_MapCommonBuffer( dBytes, pStatus );
	}
}


ARCCAM_API void
ArcCam_Close()
{
	try
	{
		if ( pCArcDev.get() != NULL )
		{
			pCArcDev.get()->Close();
		}
	}
	catch ( ... )
	{
	}
}


ARCCAM_API void
ArcCam_Reset( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->Reset(), pStatus )
}


ARCCAM_API void
ArcCam_MapCommonBuffer( int dBytes, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->MapCommonBuffer( dBytes ), pStatus )
}


ARCCAM_API void
ArcCam_UnMapCommonBuffer( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->UnMapCommonBuffer(), pStatus )
}


ARCCAM_API void
ArcCam_ReMapCommonBuffer( int dBytes, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->ReMapCommonBuffer( dBytes ), pStatus )
}


ARCCAM_API int
ArcCam_GetCommonBufferProperties( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_BOOL(
			pCArcDev.get()->GetCommonBufferProperties(),
			pStatus )
}


ARCCAM_API void
ArcCam_FillCommonBuffer( unsigned short u16Value, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->FillCommonBuffer( u16Value ), pStatus )
}


ARCCAM_API void*
ArcCam_CommonBufferVA( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_PTR( pCArcDev.get()->CommonBufferVA(), pStatus )
}


ARCCAM_API unsigned long
ArcCam_CommonBufferPA( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_ULONG( pCArcDev.get()->CommonBufferPA(), pStatus )
}


ARCCAM_API int
ArcCam_CommonBufferSize( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_INT( pCArcDev.get()->CommonBufferSize(), pStatus )
}


ARCCAM_API int
ArcCam_GetId( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_INT( pCArcDev.get()->GetId(), pStatus )
}


ARCCAM_API int
ArcCam_GetStatus( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_INT( pCArcDev.get()->GetStatus(), pStatus )
}


ARCCAM_API void
ArcCam_ClearStatus( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->ClearStatus(), pStatus )
}


ARCCAM_API void
ArcCam_Set2xFOTransmitter( int dOnOff, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH(
		pCArcDev.get()->Set2xFOTransmitter(
							static_cast<bool>( dOnOff ) ), pStatus )
}


ARCCAM_API void
ArcCam_LoadDeviceFile( const char* pszFile, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->LoadDeviceFile( pszFile ), pStatus )
}


ARCCAM_API int
ArcCam_Command( int dBoardId, int dCommand, int dArg1, int dArg2, int dArg3, int dArg4, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_INT(
			pCArcDev.get()->Command( dBoardId,
									 dCommand,
									 dArg1,
									 dArg2,
									 dArg3,
									 dArg4 ), pStatus )
}


ARCCAM_API int
ArcCam_Cmd1Arg( int dBoardId, int dCommand, int dArg1, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_INT(
			pCArcDev.get()->Command( dBoardId,
									 dCommand,
									 dArg1 ), pStatus )
}

ARCCAM_API int
ArcCam_Cmd2Arg( int dBoardId, int dCommand, int dArg1, int dArg2, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_INT(
			pCArcDev.get()->Command( dBoardId,
									 dCommand,
									 dArg1,
									 dArg2 ), pStatus )
}

ARCCAM_API int
ArcCam_Cmd3Arg( int dBoardId, int dCommand, int dArg1, int dArg2, int dArg3, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_INT(
			pCArcDev.get()->Command( dBoardId,
									 dCommand,
									 dArg1,
									 dArg2,
									 dArg3 ), pStatus )
}


ARCCAM_API int
ArcCam_GetControllerId( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_INT( pCArcDev.get()->GetControllerId(),
				   pStatus )
}


ARCCAM_API void
ArcCam_ResetController( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->ResetController(),
			   pStatus )
}


ARCCAM_API int
ArcCam_IsControllerConnected( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_BOOL( pCArcDev.get()->IsControllerConnected(),
					pStatus )
}

ARCCAM_API void
ArcCam_SetupController( int dReset, int dTdl, int dPower, int dRows, int dCols, const char* pszTimFile,
					    const char* pszUtilFile, const char* pszPciFile, int* pAbort, struct ArcCAPIStatus* pStatus )
{
	bool bAbort = false;

	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH(
			pCArcDev.get()->SetupController( static_cast<bool>( dReset ),
										     static_cast<bool>( dTdl ),
											 static_cast<bool>( dPower ),
											 dRows,
											 dCols,
											 pszTimFile,
											 pszUtilFile,
											 pszPciFile,
											 ( pAbort != NULL ? static_cast<bool>( *pAbort ) : bAbort ) ),
											 pStatus )
}


ARCCAM_API void
ArcCam_LoadControllerFile( const char* pszFilename, int dValidate, int* pAbort, struct ArcCAPIStatus* pStatus )
{
	bool bAbort = false;

	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->LoadControllerFile( pszFilename,
												   static_cast<bool>( dValidate ),
												   ( pAbort != NULL ? static_cast<bool>( *pAbort ) : bAbort ) ),
												   pStatus )
}


ARCCAM_API void
ArcCam_SetImageSize( int dRows, int dCols, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->SetImageSize( dRows,
											 dCols ),
											 pStatus )
}


ARCCAM_API int
ArcCam_GetImageRows( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_INT( pCArcDev.get()->GetImageRows(),
				   pStatus )
}


ARCCAM_API int
ArcCam_GetImageCols( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_INT( pCArcDev.get()->GetImageCols(),
				   pStatus )
}


ARCCAM_API int
ArcCam_GetCCParams( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_INT( pCArcDev.get()->GetCCParams(),
				   pStatus )
}


ARCCAM_API int
ArcCam_IsCCParamSupported( int dParameter, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_BOOL( pCArcDev.get()->IsCCParamSupported( dParameter ),
				    pStatus )
}

ARCCAM_API int
ArcCam_IsCCD( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_BOOL( pCArcDev.get()->IsCCD(),
				    pStatus )
}


ARCCAM_API int
ArcCam_IsBinningSet( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_BOOL( pCArcDev.get()->IsBinningSet(),
				    pStatus )
}


ARCCAM_API void
ArcCam_SetBinning( int dRows, int dCols, int dRowFactor, int dColFactor, int* pBinRows, int* pBinCols, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->SetBinning( dRows,
										   dCols,
										   dRowFactor,
										   dColFactor,
										   pBinRows,
										   pBinCols ),
										   pStatus )
}


ARCCAM_API void
ArcCam_UnSetBinning( int dRows, int dCols, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->UnSetBinning( dRows, dCols ), pStatus );
}


ARCCAM_API void
ArcCam_SetSubArray( int* pOldRows, int* pOldCols, int dRow, int dCol, int dSubRows, int dSubCols,
				    int dBiasOffset, int dBiasWidth, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->SetSubArray(	*pOldRows,
											*pOldCols,
											 dRow,
											 dCol,
											 dSubRows,
											 dSubCols,
											 dBiasOffset,
											 dBiasWidth ),
											 pStatus )
}


ARCCAM_API void
ArcCam_UnSetSubArray( int dRows, int dCols, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->UnSetSubArray( dRows, dCols ), pStatus )
}


ARCCAM_API int
ArcCam_IsSyntheticImageMode( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_BOOL( pCArcDev.get()->IsSyntheticImageMode(), pStatus )
}


ARCCAM_API void
ArcCam_SetSyntheticImageMode( int dMode, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH(
		pCArcDev.get()->SetSyntheticImageMode( static_cast<bool>( dMode ) ),
		pStatus )
}


ARCCAM_API void
ArcCam_SetOpenShutter( int dShouldOpen, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH(
		pCArcDev.get()->SetOpenShutter( static_cast<bool>( dShouldOpen ) ),
		pStatus )
}


ARCCAM_API void
ArcCam_Expose( float fExpTime, int dRows, int dCols, int* pAbort, void ( *pExposeCall )( float ),
			   void ( *pReadCall )( int ), int dOpenShutter, struct ArcCAPIStatus* pStatus )
{
	bool bAbort = false;

	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	IFExpose ExpIFace( pExposeCall, pReadCall );

	TRY_CATCH(
		pCArcDev.get()->Expose( fExpTime,
								dRows,
								dCols,
								( pAbort != NULL ? static_cast<const bool&>( *pAbort ) : bAbort ),
								&ExpIFace,
								static_cast<bool>( dOpenShutter ) ),
								pStatus )
}


ARCCAM_API void
ArcCam_StopExposure( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->StopExposure(),
			   pStatus )
}

ARCCAM_API void
ArcCAPI_Continuous( int dRows, int dCols, int dNumOfFrames, float fExpTime, int* pAbort,
				    void ( *pFrameCall )( int, int, int, int, void * ), int dOpenShutter,
					struct ArcCAPIStatus* pStatus )
{
	bool bAbort = false;

	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	IFConExp ExpIFace( pFrameCall );

	TRY_CATCH(
		pCArcDev.get()->Continuous( dRows,
									dCols,
									dNumOfFrames,
									fExpTime,
									( pAbort != NULL ? static_cast<const bool&>( *pAbort ) : bAbort ),
									&ExpIFace,
									static_cast<bool>( dOpenShutter ) ),
									pStatus )
}


ARCCAM_API void
ArcCam_StopContinuous( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH( pCArcDev.get()->StopContinuous(),
			   pStatus )
}


ARCCAM_API int
ArcCam_IsReadout( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_BOOL( pCArcDev.get()->IsReadout(),
					pStatus )
}


ARCCAM_API int
ArcCam_GetPixelCount( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_INT( pCArcDev.get()->GetPixelCount(),
				   pStatus )
}


ARCCAM_API int
ArcCam_GetCRPixelCount( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_INT( pCArcDev.get()->GetCRPixelCount(),
				   pStatus )
}


ARCCAM_API int
ArcCam_GetFrameCount( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_INT( pCArcDev.get()->GetFrameCount(),
				   pStatus )
}


ARCCAM_API int
ArcCam_ContainsError( int dWord )
{
	if ( pCArcDev.get() != NULL )
	{
		return pCArcDev.get()->ContainsError( dWord );
	}
	else
	{
		return 0;
	}
}


ARCCAM_API int
ArcCam_ContainsMinMaxError( int dWord, int dWordMin, int dWordMax )
{
	if ( pCArcDev.get() != NULL )
	{
		return pCArcDev.get()->ContainsError( dWord,
											  dWordMin,
											  dWordMax );
	}
	else
	{
		return 0;
	}
}


ARCCAM_API const char*
ArcCam_GetNextLoggedCmd()
{
	if ( pCArcDev.get() != NULL )
	{
		return pCArcDev.get()->GetNextLoggedCmd();
	}
	else
	{
		return NULL;
	}
}


ARCCAM_API int
ArcCam_GetLoggedCmdCount()
{
	if ( pCArcDev.get() != NULL )
	{
		return pCArcDev.get()->GetLoggedCmdCount();
	}
	else
	{
		return 0;
	}
}


ARCCAM_API void
ArcCam_SetLogCmds( int dOnOff )
{
	if ( pCArcDev.get() != NULL )
	{
		pCArcDev.get()->SetLogCmds( static_cast<bool>( dOnOff ) );
	}
}


ARCCAM_API double
ArcCam_GetArrayTemperature( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_DOUBLE(
			pCArcDev.get()->GetArrayTemperature(),
			pStatus )
}


ARCCAM_API double
ArcCam_GetArrayTemperatureDN( struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH_DOUBLE(
			pCArcDev.get()->GetArrayTemperatureDN(),
			pStatus )
}


ARCCAM_API void
ArcCam_SetArrayTemperature( double gTempVal, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH(
		 pCArcDev.get()->SetArrayTemperature( gTempVal ),
		 pStatus )
}


ARCCAM_API void
ArcCam_LoadTemperatureCtrlData( const char* pszFilename, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH(
		pCArcDev.get()->LoadTemperatureCtrlData( pszFilename ),
		pStatus )
}


ARCCAM_API void
ArcCam_SaveTemperatureCtrlData( const char* pszFilename, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CArcDevice", pCArcDev.get(), pStatus )

	TRY_CATCH(
		pCArcDev.get()->SaveTemperatureCtrlData( pszFilename ),
		pStatus )
}


ARCCAM_API void
ArcCam_Deinterlace( void *pData, int dRows, int dCols, int dAlgorithm, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CDeinterlace", pCArcDev.get(), pStatus )

	TRY_CATCH(
		pCDLacer.get()->RunAlg( pData, dRows, dCols, dAlgorithm ),
		pStatus )
}


ARCCAM_API void
ArcCam_DeinterlaceWithArg( void *pData, int dRows, int dCols, int dAlgorithm, int dArg, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CDeinterlace", pCDLacer.get(), pStatus )

	TRY_CATCH(
		pCDLacer.get()->RunAlg( pData, dRows, dCols, dAlgorithm, dArg ),
		pStatus )
}


ARCCAM_API void
ArcCam_SubtractImageHalves( void* pMem, int dRows, int dCols, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CImage", pCImage.get(), pStatus )

	TRY_CATCH(
		pCImage.get()->SubtractImageHalves( pMem, dRows, dCols ),
		pStatus )
}


ARCCAM_API void
ArcCam_VerifyImageAsSynthetic( void* pMem, int dRows, int dCols, struct ArcCAPIStatus* pStatus )
{
	VERIFY_CLASS_PTR( "CImage", pCImage.get(), pStatus )

	TRY_CATCH(
		pCImage.get()->VerifyImageAsSynthetic( pMem, dRows, dCols ),
		pStatus )
}


ARCCAM_API void
ArcCam_WriteToFitsFile( const char* pszFilename, void* pData, int dRows, int dCols, struct ArcCAPIStatus* pStatus )
{
	CFitsFile cFits( pszFilename, dRows, dCols );

	TRY_CATCH( cFits.Write( pData ), pStatus )
}

