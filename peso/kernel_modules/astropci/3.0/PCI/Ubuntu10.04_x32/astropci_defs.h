/******************************************************************************
*   File:       astropci_defs.h
*   Author:     Marco Bonati, modified by Scott Streit and Michael Ashley
*   Abstract:   Linux device driver for the SDSU PCI Interface Board. This file
*               contains the initialization and all user defined functions for
*               the driver.
*
*
*   Revision History:     Date      Who   Version    Description
*   --------------------------------------------------------------------------
*                       06/16/00    sds     1.3      Removed misc defines no
*                                                    longer used by the driver.
*                       07/11/00    sds     1.4      Added function prototype
*                                                    for astropci_read_reply().
*                       01/26/01    sds     1.4      Just changed some values.
*                       07/13/01    sds     1.6      Removed a lot of code to
*                                                    support version 1.6.
*                       09-Jan-2003 mcba    1.7B     Removed lots more code.
*
*   Development notes:
*   ---------------------------------------------------------------------------
*   This driver has been tested on Redhat Fedora Core 4, Kernel 2.6.11.
*
*   MULTIPLE PCI BOARD NOTE:
*   ---------------------------------------------------------------------------
*   This driver supports two ARC PCI boards by default. To support more boards,
*   you must modify the MAX_DEV parameter in "astropci_defs.h" and edit the
*   files "astropci_load" and "astropci_unload".
*
******************************************************************************/
#ifndef _ASTROPCI_DEFS_H
#define _ASTROPCI_DEFS_H

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,24)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

/******************************************************************************
        MULTIPLE PCI BOARD NOTE:

        This driver supports two ARC PCI boards by default. To support more
	  boards, you must modify the MAX_DEV parameter here and edit the files
        "astropci_load" and "astropci_unload".
******************************************************************************/
#define MAX_DEV               2   		// Max number of devices (PCI boards)

/******************************************************************************
        General Definitions
******************************************************************************/
#define DRIVER_NAME		( char * )"astropci"
#define DRIVER_VERSION		( char * )"2.0 Apr 30, 2008"
#define SUPPORTED_KERNELS	( char * )"2.6.24 (Fedora Core 8)"
#define ARC_PCI_DEVICE_ID     	0x1801
#define REGS_SIZE             	( 0x9C/sizeof( uint32_t ) )*sizeof( uint32_t )
#define CMD_MAX			6
#define DEBUG_ON

#define INPUT_FIFO  		0  		// For astropci_wait_for_condition()
#define OUTPUT_FIFO 		1
#define CHECK_REPLY 		2

#define CFG_OFFSET			0
#define CFG_VALUE			1

/******************************************************************************
        Debug Print Definitions
******************************************************************************/
#ifdef DEBUG_ASTROPCI
        #define PDEBUG(fmt, args...) printk ( KERN_WARNING fmt, ## args )
#else
        #define PDEBUG(fmt, args...)
#endif

#define PPDEBUG(fmt, args...)
//#define astropci_printf(fmt, args...) printk( "<1>astropci: " fmt, ## args )
#define astropci_printf(fmt, args...) printk( fmt, ## args )

/******************************************************************************
        PCI DSP Control Registers
******************************************************************************/
#define HCTR			0x10	// Host interface control register
#define HSTR			0x14	// Host interface status register 
#define HCVR			0x18	// Host command vector register   
#define REPLY_BUFFER		0x1C	// Reply Buffer				
#define CMD_DATA		0x20	// DSP command register           

/*************************************************************************** 
        State Structure - Driver state structure. All state variables related
                                  to the state of the driver are kept in here.
***************************************************************************/
typedef struct astropci_dev
{
	struct pci_dev	*pdev;			// PCI device structure
	struct cdev	 cdev;			// Char device structure
	struct semaphore sem;			// Semaphore to protect non-global access. i.e. ioctl commands
	long		 ioaddr;		// PCI I/O start address (HSTR, etc)
	long		 irqDevNum;		// IRQ device number
	uint8_t		 irq;			// PCI board IRQ level
	uint8_t		 has_irq;		// 1 if IRQ is set, 0 otherwise
	uint8_t		 has_been_probed;	// 1 if already setup by probe(), else 0
	unsigned long	 imageBufferVirtAddr;	// Virtual start address of image buffer
	uint32_t	 imageBufferPhysAddr;	// Physical start address of image buffer
	uint32_t	 imageBufferSize;	// Image buffer size (bytes)
	short		 opened;		// 1 if the driver is in use
	char		 name[ 20 ];		// Device name for probing purposes
	void            *pBootBuffer;
} astropci_dev_t;

#endif
