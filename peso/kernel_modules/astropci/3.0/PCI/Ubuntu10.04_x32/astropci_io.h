/******************************************************************************
*   File:       astropci_io.h
*   Author:     Scott Streit
*   Abstract:   I/O control commands for Linux device driver for the SDSU PCI
*               Interface Card. These commands are for use with ioctl().
*
*
*   Revision History:     Date      Who   Version    Description
*   --------------------------------------------------------------------------
*                       11/12/99    sds     1.0      Initial Release
*                       07/11/00    sds     1.4      Added binning constants.
*                       12-Jan-2002 mcba    1.7B     Removed unnecessary code.
******************************************************************************/ 
#ifndef __ASTROPCI_IO_H__
#define __ASTROPCI_IO_H__


/*********************************************
  Define macro to create ioctl command values
  Forms a value of the form: 'ARC'<cmd>
*********************************************/
#define MKCMD( cmd )	( 0x41524300 | cmd )
#define EXCMD( cmd )	( cmd & 0x000000FF )


/*********************************************
        Information retrieval commands
*********************************************/
#define ASTROPCI_GET_HCTR			0x1
#define ASTROPCI_GET_PROGRESS		0x2
#define ASTROPCI_GET_DMA_ADDR		0x3
#define ASTROPCI_GET_HSTR			0x4
#define ASTROPCI_GET_DMA_SIZE		0x6
#define ASTROPCI_GET_FRAMES_READ	0x7


/*********************************************
        Information set commands
*********************************************/
#define ASTROPCI_HCVR_DATA			0x10
#define ASTROPCI_SET_HCTR			0x11
#define ASTROPCI_SET_HCVR			0x12
#define ASTROPCI_PCI_DOWNLOAD		0x13
#define ASTROPCI_PCI_DOWNLOAD_WAIT	0x14
#define ASTROPCI_COMMAND			0x15

#define ASTROPCI_GET_CONFIG_BYTE	0x30
#define ASTROPCI_GET_CONFIG_WORD	0x31
#define ASTROPCI_GET_CONFIG_DWORD	0x32
#define ASTROPCI_SET_CONFIG_BYTE	0x33
#define ASTROPCI_SET_CONFIG_WORD	0x34
#define ASTROPCI_SET_CONFIG_DWORD	0x35


/*********************************************

        Generic Constants
*********************************************/
#define DEBUG				 		0
#define REPLY_BUFFER_EMPTY			-1


/*********************************************
        DSP Fifo Check Constants
*********************************************/
#define INPUT_FIFO_OK_MASK			0x00000002
#define OUTPUT_FIFO_OK_MASK			0x00000004
#define HTF_BIT_MASK				0x00000038
#define BUSY_WAIT_DELAY         	1           	// usec
#define SLEEP_WAIT_DELAY        	10000       	// jiffies
#define BUSY_MAX_WAIT           	1000        	// usec
#define SLEEP_MAX_WAIT          	800000      	// usec
#define REGISTER_ACCESS_DELAY		200		// nsec


/*********************************************
        Vector Commands
*********************************************/
#define READ_REPLY_VALUE			0x83
#define CLEAR_REPLY_FLAGS			0x85
#define WRITE_PCI_ADDRESS			0x8D
#define CLEAR_INTERRUPT				0x8073
#define READ_PCI_IMAGE_ADDR			0x8075
#define ABORT_READOUT				0x8079
#define READ_NUMBER_OF_FRAMES_READ	0x807D
#define PCI_DOWNLOAD				0x802F
#define WRITE_COMMAND				0xB1


/*********************************************
        DSP Replies
*********************************************/
#define DON							0x00444F4E
#define RDR							0x00524452
#define ERR							0x00455252
#define SYR							0x00535952
#define TIMEOUT						0x544F5554
#define READOUT						0x524F5554


/*********************************************
        Interrupt constants
*********************************************/
#define DMA_INTERRUPTING			0x40


/*********************************************
        Device constants
*********************************************/
#define ASTROPCI_DEVICE_ID			0x1801
#define ASTROPCI_VENDOR_ID			0x1057


/********************************************
	PCI Reply/Status Definitions
********************************************/
enum
{
	TIMEOUT_STATUS = 0,
	DONE_STATUS,
	READ_REPLY_STATUS,
	ERROR_STATUS,
	SYSTEM_RESET_STATUS,
	READOUT_STATUS,
	BUSY_STATUS
};

#endif

