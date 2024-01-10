#ifndef _CARC_PCIBASE_H_
#define _CARC_PCIBASE_H_

#include <vector>
#include <string>
#include "CArcDevice.h"
#include "CStringList.h"
#include "DllMain.h"



namespace arc
{
	//
	// PCI configuration space register data value
	//
	typedef struct PCI_REG_DATA
	{
		CStringList*	pBitList;
		std::string		sName;
		int				dValue;
		int				dAddr;
	} PCIRegData;

	typedef std::vector<PCIRegData*> PCIRegList;


	//
	// PCI configuration space register bar item ( used
	// to name and contain a set of local device configuration
	// registers; if they exist )
	//
	typedef struct PCI_BAR_DATA
	{
		std::string sName;
		PCIRegList* pList;
	} PCIBarData;

	typedef std::vector<PCIBarData*> PCIBarList;


	//
	// PCI Base Class exported in CArcDevice.dll
	//
	class CARCDEVICE_API CArcPCIBase : public CArcDevice
	{
		public:
			CArcPCIBase();
			virtual ~CArcPCIBase();

			virtual int  GetCfgSpByte( int dOffset )  = 0;
			virtual int  GetCfgSpWord( int dOffset )  = 0;
			virtual int  GetCfgSpDWord( int dOffset ) = 0;

			virtual void SetCfgSpByte( int dOffset, int dValue )  = 0;
			virtual void SetCfgSpWord( int dOffset, int dValue )  = 0;
			virtual void SetCfgSpDWord( int dOffset, int dValue ) = 0;

			virtual void GetCfgSp();
			virtual void GetBarSp();

			int          GetCfgSpCount();
			int          GetCfgSpAddr( int dIndex );
			int          GetCfgSpValue( int dIndex );
			std::string  GetCfgSpName( int dIndex );
			std::string* GetCfgSpBitList( int dIndex, int& pCount );	// User must free returned array

			int          GetBarCount();
			std::string  GetBarName( int dIndex );

			int          GetBarRegCount( int dIndex );
			int          GetBarRegAddr( int dIndex, int dRegIndex );
			int          GetBarRegValue( int dIndex, int dRegIndex );
			std::string  GetBarRegName( int dIndex, int dRegIndex );

			int          GetBarRegBitListCount( int dIndex, int dRegIndex );
			std::string  GetBarRegBitListDef( int dIndex, int dRegIndex, int dBitListIndex );

			std::string* GetBarRegBitList( int dIndex, int dRegIndex, int& pCount );	// User must free returned array

			void PrintCfgSp();
			void PrintBars();

		protected:
			void AddRegItem( PCIRegList* pvDataList, int dAddr, const char* szName, int dValue, CStringList* pBitList = NULL );
			void ClearCfgSpList();

			void AddBarItem( std::string qsName, PCIRegList* pList );
			void ClearBarList();

			CStringList* GetDevVenBitList( int dData, bool bDrawSeparator = false );
			CStringList* GetCommandBitList( int dData, bool bDrawSeparator = false );
			CStringList* GetStatusBitList( int dData, bool bDrawSeparator = false );
			CStringList* GetClassRevBitList( int dData, bool bDrawSeparator = false );
			CStringList* GetBistHeaderLatencyCache( int dData, bool bDrawSeparator = false );
			CStringList* GetBaseAddressBitList( int dData, bool bDrawSeparator = false );
			CStringList* GetSubSysBitList( int dData, bool bDrawSeparator = false );
			CStringList* GetMaxLatGntIntBitList( int dData, bool bDrawSeparator = false );

			PCIRegList*  m_pCfgSpList;
			PCIBarList*  m_pBarList;
	};

}	// end namespace


#endif		// _CARC_PCIBASE_H_
