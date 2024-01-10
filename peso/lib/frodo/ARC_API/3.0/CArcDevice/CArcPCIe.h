#ifndef _CARC_PCIE_H_
#define _CARC_PCIE_H_

#include <string>
#include <vector>
#include <memory>
#include <list>
#include "CArcPCIBase.h"
#include "DllMain.h"


namespace arc
{

	class CARCDEVICE_API CArcPCIe : public CArcPCIBase
	{
		public:
			//  Constructor/Destructor
			// +-------------------------------------------------+
			 CArcPCIe();
			virtual ~CArcPCIe();

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

			void Set2xFOTransmitter( bool bOnOff );
			void LoadDeviceFile( const char* pszFile );

			//  Setup & General commands
			// +-------------------------------------------------+
			int Command( int dBoardId, int dCommand, int dArg1 = -1, int dArg2 = -1, int dArg3 = -1, int dArg4 = -1 );

			int  GetControllerId();
			void ResetController();
			bool IsControllerConnected();
			bool IsFiberConnected( int dFiber = FIBER_A );

			//  Expose commands
			// +-------------------------------------------------+
			void StopExposure();
			bool IsReadout();
			int  GetPixelCount();
			int  GetCRPixelCount();
			int  GetFrameCount();

			//  PCIe only methods
			// +-------------------------------------------------+
			void WriteBar( int dBar, int dOffset, int dValue );
			int  ReadBar( int dBar, int dOffset );
			int  ReadReply( double fTimeOutSecs = 1.5 );

			//  Convenience names for base addr registers ( BAR )
			// +-------------------------------------------------+
			enum { LCL_CFG_BAR  = 0x00,		// Local Config Regs
				   DEV_REG_BAR  = 0x02 };	// Device Regs


			//  Device Register Offsets ( BAR2 )
			// +-------------------------------------------------+
			enum { REG_CMD_HEADER       = 0x00,
				   REG_CMD_COMMAND      = 0x04,
				   REG_CMD_ARG0         = 0x08,
				   REG_CMD_ARG1         = 0x0C,
				   REG_CMD_ARG2         = 0x10,
				   REG_CMD_ARG3         = 0x14,
				   REG_CMD_ARG4         = 0x18,
				   REG_CTLR_SPECIAL_CMD = 0x1C,
				   REG_RESET			= 0x20,
				   REG_INIT_IMG_ADDR    = 0x38,
				   REG_FIBER_2X_CTRL	= 0x5C,
				   REG_STATUS           = 0x60,
				   REG_CMD_REPLY        = 0x64,
				   REG_CTLR_ARG1		= 0x68,
				   REG_CTLR_ARG2        = 0x6C,
				   REG_PIXEL_COUNT      = 0x70,
				   REG_FRAME_COUNT      = 0x74,
				   REG_ID_LO            = 0x78,
				   REG_ID_HI            = 0x7C };


			//  Special Command Register Commands
			// +-------------------------------------------------+
			enum { CONTROLLER_GET_ID    = 0x09,
				   CONTROLLER_RESET     = 0x0B };


			//  Fiber Optic Selector
			// +-------------------------------------------------+
			enum { FIBER_A, FIBER_B };


			//  Fiber Optic 2x Selector
			// +-------------------------------------------------+
			enum { FIBER_2X_DISABLE, FIBER_2X_ENABLE };


			//  PCIe Board ID Constant
			// +-------------------------------------------------+
			static const int ID			= 0x41524336;	// 'ARC6'

		protected:
			int  SmallCamDLoad( int dBoardId, std::vector<int>& vData );
			void LoadGen23ControllerFile( const char *pszFilename, bool bValidate, const bool& bAbort = false );
			void SetByteSwapping();

			void TestMemory( int dValue );
			void GetLocalConfiguration();

			static std::vector<ArcDev_t>	m_vDevList;
			static char**					m_pszDevList;
	};


	// +----------------------------------------------------------------------------
	// |  Status Register Macros
	// +----------------------------------------------------------------------------
	#define STATUS_CLEAR_ALL				0x7F

	#define STATUS_REPLY_RECVD( dStatus )	\
						( ( dStatus & 0x00000003 ) == 2 ? true : false )

	#define STATUS_CONTROLLER_RESET( dStatus )	\
						( ( dStatus & 0x00000008 ) > 0 ? true : false )

	#define STATUS_READOUT( dStatus )	\
						( ( dStatus & 0x00000004 ) > 0 ? true : false )

	#define STATUS_IDLE( dStatus )	\
						( ( dStatus & 0x00000003 ) == 0 ? true : false )

	#define STATUS_CMD_SENT( dStatus )	\
						( ( dStatus & 0x00000003 ) == 1 ? true : false )

	#define STATUS_IMG_READ_TIMEOUT( dStatus )	\
					( ( dStatus & 0x00000020 ) > 0 ? true : false )

	#define STATUS_HDR_ERROR( dStatus )	\
						( ( dStatus & 0x00000010 ) > 0 ? true : false )

	#define STATUS_FIBER_2X_RECEIVER( dStatus ) \
						( ( dStatus & 0x00000200 ) > 0 ? true : false )

	#define STATUS_FIBER_A_CONNECTED( dStatus ) \
						( ( dStatus & 0x00000080 ) > 0 ? true : false )

	#define STATUS_FIBER_B_CONNECTED( dStatus ) \
						( ( dStatus & 0x00000100 ) > 0 ? true : false )


	// +------------------------------------------------------------------------------
	// |  Driver ioctl commands
	// +------------------------------------------------------------------------------
	#define ARC_READ_BAR		0x01			// Read PCI/e base address register
	#define ARC_WRITE_BAR		0x02			// Write PCI/e base address register
	#define ARC_BAR_SIZE		0x03			// Get PCI/e base address register size

	#define ARC_READ_CFG_8		0x04			// Read 8-bits of PCI/e config space
	#define ARC_READ_CFG_16		0x05			// Read 16-bits of PCI/e config space
	#define ARC_READ_CFG_32		0x06			// Read 32-bits of PCI/e config space

	#define ARC_WRITE_CFG_8		0x07			// Write 8-bits to PCI/e config space
	#define ARC_WRITE_CFG_16	0x08			// Write 16-bits to PCI/e config space
	#define ARC_WRITE_CFG_32	0x09			// Write 32-bits to PCI/e config space

	#define ARC_BUFFER_PROP		0x0A			// Get common buffer properties

	#define ARC_MEM_MAP			0x0C			// Maps BAR or common buffer
	#define ARC_MEM_UNMAP		0x0D			// UnMaps BAR or common buffer

}	// end namespace


#endif
