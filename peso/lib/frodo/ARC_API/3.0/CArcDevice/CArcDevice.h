#ifndef _CARC_DEVICE_H_
#define _CARC_DEVICE_H_


#include "CExpIFace.h"
#include "CConIFace.h"
#include "CLog.h"
#include "TempCtrl.h"
#include "DllMain.h"
#include "ArcOSDefs.h"

#ifdef linux
#include <sys/types.h>
#endif


namespace arc
{

	// +------------------------------------------------+
	// | Image buffer info                              |
	// +------------------------------------------------+
	typedef struct ARC_DEVICE_BUFFER
	{
		void*  pUserAddr;
		ulong  ulPhysicalAddr;
		size_t dSize;
	} ImgBuf_t;


	// +------------------------------------------------+
	// | Device info                                    |
	// +------------------------------------------------+
	typedef struct ARC_DEVICE_INFO
	{
		std::string			sName;

#ifdef __APPLE__
		io_service_t		tService;
#endif
	} ArcDev_t;


	// +------------------------------------------------+
	// | CArcDevice class definition                    |
	// +------------------------------------------------+
	class CARCDEVICE_API CArcDevice
	{
		public:
			CArcDevice( void );
			virtual ~CArcDevice( void );

			virtual const char* ToString() = 0;

			//  Device access
			// +-------------------------------------------------+
			virtual bool IsOpen();
			virtual void Open( int dDeviceNumber = 0 ) = 0;
			virtual void Open( int dDeviceNumber, int dBytes ) = 0;
			virtual void Close() = 0;
			virtual void Reset() = 0;

			virtual void  MapCommonBuffer( int dBytes = 0 ) = 0;
			virtual void  UnMapCommonBuffer() = 0;
			virtual void  ReMapCommonBuffer( int dBytes = 0 );
			virtual bool  GetCommonBufferProperties() = 0;
			virtual void  FillCommonBuffer( unsigned short u16Value = 0 );
			virtual void* CommonBufferVA();
			virtual ulong CommonBufferPA();
			virtual int   CommonBufferSize();

			virtual int  GetId() = 0;
			virtual int  GetStatus() = 0;
			virtual void ClearStatus() = 0;

			virtual void Set2xFOTransmitter( bool bOnOff ) = 0;
			virtual void LoadDeviceFile( const char* pszFile ) = 0;

			//  Setup & General commands
			// +-------------------------------------------------+
			virtual int  Command( int dBoardId, int dCommand, int dArg1 = -1, int dArg2 = -1, int dArg3 = -1, int dArg4 = -1 ) = 0;
			virtual int  GetControllerId() = 0;
			virtual void ResetController() = 0;
			virtual bool IsControllerConnected() = 0;

			void SetupController( bool bReset, bool bTdl, bool bPower, int dRows, int dCols, const char* pszTimFile,
							      const char* pszUtilFile = NULL, const char* pszPciFile = NULL, const bool& bAbort = false );

			void LoadControllerFile( const char* pszFilename, bool bValidate = true, const bool& bAbort = false );
			void SetImageSize( int dRows, int dCols );
			int  GetImageRows();
			int  GetImageCols();
			int  GetCCParams();
			bool IsCCParamSupported( int dParameter );
			bool IsCCD();

			bool IsBinningSet();
			void SetBinning( int dRows, int dCols, int dRowFactor, int dColFactor, int* pBinRows = NULL, int* pBinCols = NULL );
			void UnSetBinning( int dRows, int dCols );

			void SetSubArray( int& dOldRows, int& dOldCols, int dRow, int dCol, int dSubRows, int dSubCols, int dBiasOffset, int dBiasWidth );
			void UnSetSubArray( int dRows, int dCols );

			bool IsSyntheticImageMode();
			void SetSyntheticImageMode( bool bMode );

			//  Expose commands
			// +-------------------------------------------------+
			void SetOpenShutter( bool bShouldOpen );

			void Expose( float fExpTime, int dRows, int dCols, const bool& bAbort = false,
						 CExpIFace* pExpIFace = NULL, bool bOpenShutter = true );

			virtual void StopExposure() = 0;

			void Continuous( int dRows, int dCols, int dNumOfFrames, float fExpTime, const bool& bAbort = false,
							 CConIFace* pConIFace = NULL, bool bOpenShutter = true );

			void StopContinuous();

			virtual bool IsReadout() = 0;
			virtual int  GetPixelCount() = 0;
			virtual int  GetCRPixelCount() = 0;
			virtual int  GetFrameCount() = 0;

			//  Error & Degug message access
			// +-------------------------------------------------+
			bool ContainsError( int dWord );
			bool ContainsError( int dWord, int dWordMin, int dWordMax );

			const char* GetNextLoggedCmd();
			int         GetLoggedCmdCount();
			void        SetLogCmds( bool bOnOff );

			//  Temperature control
			// +-------------------------------------------------+
			double GetArrayTemperature();
			double GetArrayTemperatureDN();
			void   SetArrayTemperature( double gTempVal );
			void   LoadTemperatureCtrlData( const char* pszFilename );
			void   SaveTemperatureCtrlData( const char* pszFilename );

		protected:
			void   SetDefaultTemperatureValues();
			double ADUToVoltage( int dAdu, bool bArc12 = false, bool bHighGain = false );
			double VoltageToADU( double gVoltage, bool bArc12 = false, bool bHighGain = false );
			double CalculateAverageTemperature();
			double CalculateVoltage( double gTemperature );
			double CalculateTemperature( double gVoltage );

			virtual int  SmallCamDLoad( int dBoardId, std::vector<int>& vData ) = 0;
			virtual void LoadSmallCamControllerFile( const char *pszFilename, bool bValidate, const bool& bAbort = false );
			virtual void LoadGen23ControllerFile( const char *pszFilename, bool bValidate, const bool& bAbort = false ) = 0;
			virtual void SetByteSwapping() = 0;

			std::string FormatDLoadString( int dReply, int dBoardId, std::vector<int>& vData );

			//  Temperature control variables
			// +--------------------------------------+
			double gTmpCtrl_DT670Coeff1;
			double gTmpCtrl_DT670Coeff2;
			double gTmpCtrl_SDAduOffset;
			double gTmpCtrl_SDAduPerVolt;
			double gTmpCtrl_HGAduOffset;
			double gTmpCtrl_HGAduPerVolt;
			double gTmpCtrl_SDVoltTolerance;
			double gTmpCtrl_SDDegTolerance;
			int    gTmpCtrl_SDNumberOfReads;
			int    gTmpCtrl_SDVoltToleranceTrials;

			TmpCtrlCoeff_t TmpCtrl_SD_2_12K;
			TmpCtrlCoeff_t TmpCtrl_SD_12_24K;
			TmpCtrlCoeff_t TmpCtrl_SD_24_100K;
			TmpCtrlCoeff_t TmpCtrl_SD_100_475K;

			Arc_DevHandle	m_hDevice;		// Driver file descriptor
			CLog			m_cApiLog;
			ImgBuf_t 		m_tImgBuffer;
			int		 		m_dCCParam;
			bool	 		m_bStoreCmds;	// 'true' stores cmd strings in queue
	};


	// +------------------------------------------------------------------+
	// | Maximum number of command parameters the controller will accept  |
	// +------------------------------------------------------------------+
	#define CTLR_CMD_MAX	6

	// +------------------------------------------------------------------+
	// | Timeout loop count for image readout                             |
	// +------------------------------------------------------------------+
	#define READ_TIMEOUT	200

}	// end namespace


#endif	// _CARC_DEVICE_H_
