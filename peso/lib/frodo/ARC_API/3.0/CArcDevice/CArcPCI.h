#ifndef _CARC_PCI_H_
#define _CARC_PCI_H_

#include <vector>
#include <string>
#include "CArcPCIBase.h"
#include "CStringList.h"
#include "DllMain.h"


namespace arc
{

	class CARCDEVICE_API CArcPCI : public CArcPCIBase
	{
		public:
			 CArcPCI( void );
			~CArcPCI( void );

			const char* ToString();

			int  GetCfgSpByte( int dOffset );
			int  GetCfgSpWord( int dOffset );
			int  GetCfgSpDWord( int dOffset );

			void SetCfgSpByte( int dOffset, int dValue );
			void SetCfgSpWord( int dOffset, int dValue );
			void SetCfgSpDWord( int dOffset, int dValue );

			void GetCfgSp();
			void GetBarSp();

			//  Device access
			// +-------------------------------------------------+
			static void FindDevices();
			static void UseDevices( const char** pszDeviceList, int dListCount );
			static int  DeviceCount();
			static const char** GetDeviceStringList();
			static void FreeDeviceStringList();

			bool IsOpen();
			void Open( int dDeviceNumber = 0 );
			void Open( int dDeviceNumber, int dBytes );
			void Close();
			void Reset();

			bool GetCommonBufferProperties();
			void MapCommonBuffer( int dBytes = 0 );
			void UnMapCommonBuffer();

			int  GetId();
			int  GetStatus();
			void ClearStatus();
			void SetStatus( int dStatus );

			void Set2xFOTransmitter( bool bOnOff );
			void LoadDeviceFile( const char* pszFile );

			//  Setup & General commands
			// +-------------------------------------------------+
			int  Command( int dBoardId, int dCommand, int dArg1 = -1, int dArg2 = -1, int dArg3 = -1, int dArg4 = -1 );

			int  GetControllerId();
			void ResetController();
			bool IsControllerConnected();

			//  Expose commands
			// +-------------------------------------------------+
			void StopExposure();
			bool IsReadout();
			int  GetPixelCount();
			int  GetCRPixelCount();
			int  GetFrameCount();

			//  PCI only commands
			// +-------------------------------------------------+
			void SetHctr( int dVal );
			int  GetHstr();
			int  GetHctr();

			int  PCICommand( int dCommand );
			int  IoctlDevice( int dIoctlCmd, int dArg = -1 );
			int  IoctlDevice( int dIoctlCmd, int dArg[], int dArgCount );

		private:
			int  SmallCamDLoad( int dBoardId, std::vector<int>& vData );
			void LoadPCIFile( const char *c_filename, const bool& bAbort = false );
			void LoadGen23ControllerFile( const char *pszFilename, bool bValidate, const bool& bAbort = false );
			void SetByteSwapping();

			const std::string FormatPCICommand( int dCmd, int dReply, int dArg = -1, int dSysErr = -1 );
			const std::string FormatPCICommand( int dCmd, int dReply, int dArg[], int dArgCount, int dSysErr = -1 );

			CStringList* GetHSTRBitList( int dData, bool bDrawSeparator = false );

			static std::vector<ArcDev_t>	m_vDevList;
			static char**					m_pszDevList;
	};

	// +------------------------------------------------------------------------------
	// |  Driver ioctl commands
	// +------------------------------------------------------------------------------
	#define ASTROPCI_GET_HCTR				0x01
	#define ASTROPCI_GET_PROGRESS			0x02
	#define ASTROPCI_GET_DMA_ADDR			0x03
	#define ASTROPCI_GET_HSTR				0x04
	#define ASTROPCI_MEM_MAP				0x05
	#define ASTROPCI_GET_DMA_SIZE			0x06
	#define ASTROPCI_GET_FRAMES_READ		0x07
	#define ASTROPCI_HCVR_DATA				0x10
	#define ASTROPCI_SET_HCTR				0x11
	#define ASTROPCI_SET_HCVR				0x12
	#define ASTROPCI_PCI_DOWNLOAD			0x13
	#define ASTROPCI_PCI_DOWNLOAD_WAIT		0x14
	#define ASTROPCI_COMMAND				0x15
	#define ASTROPCI_MEM_UNMAP				0x16
	#define ASTROPCI_ABORT					0x17
	#define ASTROPCI_CONTROLLER_DOWNLOAD	0x19
	#define ASTROPCI_GET_CR_PROGRESS		0x20
	#define ASTROPCI_GET_DMA_LO_ADDR		0x21
	#define ASTROPCI_GET_DMA_HI_ADDR		0x22
	#define ASTROPCI_GET_CONFIG_BYTE		0x30
	#define ASTROPCI_GET_CONFIG_WORD		0x31
	#define ASTROPCI_GET_CONFIG_DWORD		0x32
	#define ASTROPCI_SET_CONFIG_BYTE		0x33
	#define ASTROPCI_SET_CONFIG_WORD		0x34
	#define ASTROPCI_SET_CONFIG_DWORD		0x35

	// +------------------------------------------------------------------------------
	// |  Status register ( HSTR ) constants
	// +------------------------------------------------------------------------------
	#define HTF_BIT_MASK					0x00000038

	enum {
		TIMEOUT_STATUS = 0,
		DONE_STATUS,
		READ_REPLY_STATUS,
		ERROR_STATUS,
		SYSTEM_RESET_STATUS,
		READOUT_STATUS,
		BUSY_STATUS
	};

	// +----------------------------------------------------------------------------
	// | PCI commands
	// +----------------------------------------------------------------------------
	#define PCI_RESET						0x8077
	#define ABORT_READOUT					0x8079
	#define BOOT_EEPROM						0x807B
	#define READ_HEADER						0x81
	#define RESET_CONTROLLER				0x87
	#define INITIALIZE_IMAGE_ADDRESS		0x91
	#define WRITE_COMMAND					0xB1

}	// end namespace

#endif	// _CARC_PCI_H_
