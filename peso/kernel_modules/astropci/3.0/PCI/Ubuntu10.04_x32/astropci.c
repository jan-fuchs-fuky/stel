/******************************************************************************
*   File:       astropci.c
*
*   Author:     Marco Bonati, modified by Scott Streit, 
*               Brian Taylor, and Michael Ashley
*
*   Abstract:   Linux device driver for the ARC PCI Interface Board.
*
*
*   Revision History:
*     
*   Date      Who Version Description
* -----------------------------------------------------------------------------
* 09-Jun-2000 sds  1.3    Initial.
*
* 11-Jul-2000 sds  1.4    Changed ioctl GET_REPLY
*                         case to use ASTROPCI_READ_REPLY()
*                         function.
*
* 13-Jul-2001 sds  1.6    Removed alot of code and added
*                         mmap to support version 1.6.
*
* 18-Sep-2001 sds  1.7    Fixed unload bug.
*
* 05-Dec-2001 sds  1.7    Added image buffer code to astropci_t
*                         struct and removed mem_base[]. Fixed
*                         PCI board probing problem. Correct boards
*                         are now found, but this could still be
*                         improved. Supports multiple boards better.
*
* 11-Jun-2002 sds  1.7    Improved mmap management system. Still
*                         cannot recover used memory very well, however.
*
* 09-Jan-2002 mcba 1.7    Using new PCI probing technique; register memory
*                         regions; spin_locks; open now enforces only
*                         one process per board; removed unnecessary includes;
*                         removed lots of code; reduced size of static structs;
*                         MAX_DEV consistency check; more use of MOD_INC/DEC_
*                         USAGE_COUNT; +ve error returns made -ve; added
*                         astropci_wait_for_condition, and made it sleep for
*                         long delays; fixed memory caching in mmap; added
*                         /proc interface; MODULE_ stuff; EXPORT_NO_SYMBOLS;
*                         LINUX_VERSION_CODE switches; removed unnecessary
*                         wait queues.
*
* 15-Apr-2003 sds  1.7    Re-added support for multiple boards. Fixed mmap
*			  			  to support 2.4.20-8 kernel.
*
* 30-Aug-2005 sds  1.7    Added Read/Write register functions, which include
*			  			  delays before reading/writing the PCI DSP registers
*			  			  (HCTR, HSTR, etc). Also includes checking bit 1 of
*			  			  the HCVR, if it's set, do not write to the HCVR register.
*			  			  Also did general cleanup, including re-writing the
*			  			  astropci_wait_for_condition function. Updated for current
*			  			  kernel PCI API.
*
* 23-Jul-2007 sds  1.7    Modified to create a driver with a single major number.
*			  			  Each PCI board is attached via a minor number. This
*			  			  driver is only valid for kernels >= 2.6.11 (Fedora Core 4).
*			  			  Also fixed problem where driver failed to unload correctly
*			  			  using astropci_unload.
*
* 30-Apr-2008 sds  2.0    Updated for FC8. Required changes to interrupt constants
*			  			  and ISR function prototype.
*
* 12-Jun-2009 sds  2.0    Updated for FC11.
*
* 24-Aug-2011 sds  3.0    Fixed PCI driver data pointer error and IRQ free problem.
*
* 12-Sep-2011 sds  3.0    Replaced ioctl with unlocked_ioctl.
*
* 26-Sep-2011 sds  2.0    Removed unused ASTROPCI_GET_CR_PROGRESS command.
*
*   Development notes:
*   ---------------------------------------------------------------------------
*   This driver has been tested on CentOS 5.x x64, Kernel 2.6.18-238.12.1.el5.
*
*   MULTIPLE PCI BOARD NOTE:
*   ---------------------------------------------------------------------------
*   This driver supports two ARC PCI boards by default. To support more boards,
*   you must modify the MAX_DEV parameter in "astropci_defs.h" and edit the
*   files "astropci_load" and "astropci_unload".
*
******************************************************************************/
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/smp_lock.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#include <linux/highmem.h>
#include <linux/memory.h>

#include <asm/uaccess.h>
#include "astropci_defs.h"
#include "astropci_io.h"

/******************************************************************************
        Global variables
******************************************************************************/
static spinlock_t	astropci_lock = SPIN_LOCK_UNLOCKED;
static astropci_dev_t	devices[ MAX_DEV ];
static uint32_t		nextValidStartAddress = 0;
static int		astropci_major = 0;

/******************************************************************************
        Prototypes for main entry points
******************************************************************************/
static int  astropci_open( struct inode *, struct file * );
static int  astropci_close( struct inode *, struct file * );
static long astropci_ioctl( struct file *, unsigned int, unsigned long );
static int  astropci_mmap( struct file *file, struct vm_area_struct *vma );

/******************************************************************************
        Prototypes for other functions
******************************************************************************/
static int __devinit astropci_probe( struct pci_dev *pdev, const struct pci_device_id *ent );
static void __devexit astropci_remove( struct pci_dev *pdev );
static irqreturn_t astropci_ISR( int irq, void *dev_id );
static int astropci_flush_reply_buffer( int devnum );
static int astropci_check_reply_flags( int devnum );
static int astropci_check_dsp_input_fifo( int devnum );
static int astropci_check_dsp_output_fifo( int devnum );
static uint32_t astropci_wait_for_condition( int devnum, int condition_to_wait_for );
static int astropci_set_buffer_addresses( int devnum );
static unsigned int ReadRegister_32( long addr );
static unsigned short ReadRegister_16( long addr );
static void WriteRegister_32( unsigned int value, long addr );
static void WriteRegister_16( unsigned short value, long addr );
static unsigned int Read_HCTR( int devnum );
static unsigned int Read_HSTR( int devnum );
static unsigned int Read_REPLY_BUFFER_32( int devnum );
static unsigned short Read_REPLY_BUFFER_16( int devnum );
static void Write_HCTR( int devnum, unsigned int regVal );
static void Write_CMD_DATA_32( int devnum, unsigned int regVal );
static void Write_CMD_DATA_16( int devnum, unsigned short regVal );
static int Write_HCVR( int devnum, unsigned int regVal );

static struct class *astropci_class;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,5)
static int __init astropci_init( void );
static void __exit astropci_exit( void );
#else
int init_module( void );
void cleanup_module( void );
#endif

/******************************************************************************
   As of kernel 2.4, you can rename the init and cleanup functions. They
   no longer need to be called init_module and cleanup_module. The macros
   module_init and module_exit, found in linux/init.h, can be used to
   define your own functions. BUT, these macros MUST be called after defining
   init and cleanup functions.
******************************************************************************/
module_init( astropci_init );
module_exit( astropci_exit );

/******************************************************************************
        Structures used by the kernel PCI API
******************************************************************************/
static struct pci_device_id __devinitdata astropci_pci_id[] =
{
	{ PCI_DEVICE( PCI_VENDOR_ID_MOTOROLA, ARC_PCI_DEVICE_ID ) },
	{ 0, }
};
MODULE_DEVICE_TABLE( pci, astropci_pci_id );

static struct pci_driver astropci_driver = {
        .name      =   DRIVER_NAME,
        .id_table  =   astropci_pci_id,
        .probe     =   astropci_probe,
        .remove    =   __devexit_p( astropci_remove ),
};

/******************************************************************************
        file_operations - character/block entry points structure. Contains
                          entry points for drivers that support both character
                          and block entry points. We only specify the functions
                          that we provide.
******************************************************************************/
static struct file_operations astropci_fops = {
        .owner			= THIS_MODULE,
        .unlocked_ioctl	= astropci_ioctl,
        .mmap			= astropci_mmap,
        .open			= astropci_open,
        .release		= astropci_close
};

/******************************************************************************
 FUNCTION: INIT_MODULE()
 
 PURPOSE:  Initializes the module.
 
 NOTES:    Called by Linux during insmod, not by the user.
******************************************************************************/
static int __init astropci_init( void )
{
	int result  = 0;
	int  minor  = 0;
	long devnum = 0;
	dev_t dev  = MKDEV( astropci_major, 0 );

	#if LINUX_VERSION_CODE < KERNEL_VERSION( 2,6,11 )
	astropci_printf( "(astropci_init): invalid kernel version, must be 2.6.11 or greater!\n" );
	return -1;
	#endif

	astropci_printf( "(astropci_init): +-------------------------------+\n" );
	astropci_printf( "(astropci_init): |   ASTROPCI - Initialization   |\n" );
	astropci_printf( "(astropci_init): +-------------------------------+\n" );

	/* Figure out our device number. */
	result = alloc_chrdev_region( &dev, 0, MAX_DEV, DRIVER_NAME );
	astropci_major = MAJOR( dev );

	if ( result < 0 )
	{
		astropci_printf( "(astropci_init): unable to get major %d\n",
					astropci_major );
	}
	else
	{
		if ( astropci_major == 0 )
		{
			astropci_major = result;
		}

		astropci_class = class_create( THIS_MODULE, DRIVER_NAME );

		if ( IS_ERR( astropci_class ) )
		{
			int err = PTR_ERR( astropci_class );
			astropci_printf( "(astropci_init): failure creating class, error %d\n", err );
			return err;
		}

		// Now set up the cdevs
		for ( minor = 0; minor < MAX_DEV; minor++ )
		{
			memset( &devices[ minor ], 0x00, sizeof( struct astropci_dev ) );

			devnum = MKDEV( astropci_major, minor );
			cdev_init( &devices[ minor ].cdev, &astropci_fops );
			devices[ minor ].cdev.owner = THIS_MODULE;
			devices[ minor ].cdev.ops = &astropci_fops;
			devices[ minor ].has_been_probed = 0;
			devices[ minor ].has_irq = 0;
			devices[ minor ].opened = 0;
			devices[ minor ].irqDevNum = minor + 1;
			sprintf( devices[ minor ].name, "%s%d", DRIVER_NAME, minor );

			result = cdev_add( &devices[ minor ].cdev, devnum, 1 );

			// Fail gracefully if need be
			if ( result )
			{
				astropci_printf( "(astropci_init): Error %d adding device%d",
							result, minor );
			}
		}

		if ( !result )
		{
			result = pci_register_driver( &astropci_driver );
		}
	}

	return result;
}

/******************************************************************************
 FUNCTION: CLEANUP_MODULE()
 
 PURPOSE:  Remove all resources allocated by init_module().
 
 NOTES:    Called by Linux during rmmod, not by the user.
******************************************************************************/
static void __exit astropci_exit( void )
{
	int i;

	for ( i=0; i<MAX_DEV; i++ )
	{
		// To prevent confusion, only print messages for boards
		// that have been successfully probed.
		if ( devices[ i ].has_been_probed == 1 )
		{
			astropci_printf( "(astropci_exit): unregistering device #%d\n", i );
		}

		cdev_del( &devices[ i ].cdev );
		device_destroy( astropci_class, MKDEV( astropci_major, i ) );
	}

	class_destroy( astropci_class );
	unregister_chrdev_region( MKDEV( astropci_major, 0 ), MAX_DEV );
	pci_unregister_driver ( &astropci_driver );

 	astropci_printf( "(astropci_exit): driver unloaded\n" );
}

/******************************************************************************
 FUNCTION: ASTROPCI_PROBE()
 
 PURPOSE:  This function is called by pci_register_driver for each PCI device
           that matches the pci_driver struct.
 
 RETURNS:  Returns 0 for success, or the appropriate error number.
 
 NOTES:    See Documentation/pci.txt in the Linux sources.
******************************************************************************/
static int __devinit 
astropci_probe ( struct pci_dev *pdev, const struct pci_device_id *ent )
{
	int  result  = 0;
	long devnum  = 0;
	long memaddr = 0;
	astropci_dev_t *device = NULL;

	astropci_printf( "(astropci_probe): looking for ARC PCI boards ...\n" );

	// Initialize device before it's used by the driver
	result = pci_enable_device( pdev );

	if ( result == 0 )
	{
		// Find an available device
		for ( devnum = 0; devnum < MAX_DEV; devnum++ )
		{
			if ( !devices[ devnum ].has_been_probed )
			{
				device = &devices[ devnum ];
				devices[ devnum ].has_been_probed = 1; 
				break;
			}	
		}

		if ( device == NULL )
		{
			astropci_printf( "(astropci_probe): number of PCI boards exceeds " );
			astropci_printf( "maximum supported by driver (%d). Please edit ", MAX_DEV );
			astropci_printf( "the MAX_DEV variable and re-compile driver.\n" );
			result = -ENODEV;
		}
	}

	if ( result == 0 )
	{
		// Mark all PCI I/O regions associated with the PCI device
		// as being reserved by the specified owner.
		result = pci_request_regions( pdev, ( char * )device->name );

		if ( result == 0 )
		{

			// Set the private data field in pdev, so that we can determine
			// which board is referred to. Note that we are storing the 
			// current value of dev, not a pointer to it.
			pci_set_drvdata( pdev, &device->irqDevNum );

			// Save the PCI device structure
			device->pdev = pdev;

			// Get the start address of the PCI's I/O resources.
			memaddr = pci_resource_start( pdev, 0 );
			device->ioaddr = ( long )ioremap_nocache( memaddr, REGS_SIZE );

			// Move on if we successfully got the PCI's I/O address.
			if ( device->ioaddr )
			{
				astropci_printf( "(astropci_probe): ISR name: %s device num: %ld\n",
						  device->name, devnum );

				// Install the interrupt service routine. The pci_dev structure
				// that's passed in already contains the IRQ read from the PCI
				// board. Uses shared interrupts.
				//
				// NOTE: the last parameter ( void *dev_id ) CANNOT be NULL for
				// shared interrupts, which is what is used by the PCI boards.
				result = request_irq( pdev->irq,
					    	      &astropci_ISR,
					    	      IRQF_SHARED,		// SA_INTERRUPT | SA_SHIRQ,
					    	      device->name,
					    	      &device->irqDevNum );

				// IMPORTANT - DO NOT mess with the enable_irq and disable_irq
				// functions when using shared interrupts, because these will
				// disable the interrupt for all devices using the IRQ, which
				// may cause problems for other devices, such as the NIC.

				if ( result == 0 )
				{
					device->has_irq = 1;

					// Enable the device interrupts
        				astropci_printf( "(astropci_probe): Successfully probed " );
					astropci_printf( "device %ld, irqDevNum: %ld, has_irq: %d, irq %d, func %d\n",
							  devnum, device->irqDevNum, device->has_irq, pdev->irq, pdev->devfn );

					device_create( astropci_class,
								   NULL,
								   MKDEV( astropci_major, devnum ),
								   &device->irqDevNum,
								   "astropci%ld",
								   devnum );
				}
				else
				{
					astropci_printf( "(astropci_probe): Request IRQ %d failed\n",
								pdev->irq );
				}
			}
			else
			{
				astropci_printf( "(astropci_probe): IOremap failed for device %s, \
							region 0x%X @ 0x%lX\n", pci_name( pdev ),
							REGS_SIZE, memaddr );
			}
		}
	}

	// Undo everything if we failed.
	if ( result != 0 )
	{
		iounmap( ( void * )device->ioaddr );
		pci_release_regions( pdev );
	}

	astropci_printf( "(astropci_probe): finished looking for ARC PCI boards\n" );

	return result;
}

/******************************************************************************
 FUNCTION: ASTROPCI_REMOVE()
 
 PURPOSE:  This function is called by pci_unregister_driver for each 
           PCI device that was successfully probed.
 
 NOTES:    See Documentation/pci.txt in the Linux sources.
******************************************************************************/
static void __devexit astropci_remove( struct pci_dev *pdev )
{
	long* pIrqDevNum = 0;
	long  devnum     = 0;
 
	if ( pdev == NULL )
	{
		astropci_printf( "astropci (remove): pdev parameter is NULL!\n" );
		return;
	}

	pIrqDevNum = ( long * )pci_get_drvdata( pdev );
	devnum = *pIrqDevNum - 1;

	astropci_printf( "astropci (remove): removing device #: %ld\n", devnum );

	if ( ( devnum >= 0 ) && ( devnum < MAX_DEV ) && ( ( devnum + 1 ) == devices[ devnum ].irqDevNum ) )
	{
		if ( devices[ devnum ].has_irq == 1 )
		{
			devices[ devnum ].has_irq = 0;
			free_irq( pdev->irq, pIrqDevNum );
		}

		if ( devices[ devnum ].ioaddr != 0 )
		{
			iounmap( ( void * )( devices[ devnum ].ioaddr ) );
		}

		pci_release_regions( pdev );
		pci_disable_device( pdev );

		pci_set_drvdata( pdev, ( void * )-1 );
	}
	else
	{
		astropci_printf( "astropci (remove): bad device number %ld\n", devnum );
	}
}

/******************************************************************************
 FUNCTION: ASTROPCI_OPEN()
 
 PURPOSE:  Entry point. Open a device for access.
 
 RETURNS:  Returns 0 for success, or the appropriate error number.
******************************************************************************/
static int astropci_open( struct inode *inode, struct file *file )
{
	uint32_t value = 0;
	long devnum    = 0;
	int result     = 0;

	// We use a lock to protect global variables
	spin_lock( &astropci_lock );

	// Obtain the minor device number, which is used to determine
	// which of the possible ARC cards we want to open
	devnum = iminor( inode );

	astropci_printf( "(open): device number: %ld MAX_DEV: %d hasIRQ: %d\n",
				devnum, MAX_DEV, devices[ devnum ].has_irq );

	// Check that this device actually exists
	if ( ( devnum < 0 ) || ( devnum >= MAX_DEV ) || ( devices[ devnum ].has_irq == 0 ) )
	{
		result = -ENXIO;
	}

	if ( result == 0 )
	{
 		// Allow only one process to open the device at a time
		if ( devices[ devnum ].opened )
		{
			result = -EBUSY;
		}
	}

	if ( result == 0 )
	{
		// Write 0xFF to the configuration-space address PCI_LATENCY_TIMER
		pci_write_config_byte( devices[ devnum ].pdev, PCI_LATENCY_TIMER, 0xFF );

		// Set HCTR bit 8 to 1 and bit 9 to 0 for 32-bit PCI commands -> 24-bit DSP data
		// Set HCTR bit 11 to 1 and bit 12 to 0 for 24-bit DSP reply data -> 32-bit PCI data
		value = Read_HCTR( devnum );
		Write_HCTR( devnum, ( ( value & 0xCFF ) | 0x900 ) );

 		if ( astropci_flush_reply_buffer( devnum ) )
		{
			astropci_printf( "Flush reply buffer failed\n" );
			result = -EACCES;
		}
	}

	if ( result == 0 )
	{
		// Set the device state to opened
		devices[ devnum ].opened = 1;

		// Store the board number into the file structure,
		// so that mmap() can easily determine the board.
		file->private_data = ( void * )devnum;

		// Initialize the semaphore
		init_MUTEX( &devices[ devnum ].sem );
	}

	spin_unlock( &astropci_lock );

	// Increase the module usage count. Prevents accidental
	// unloading of the device while it's in use.
	try_module_get( THIS_MODULE );

	return result;
}

/******************************************************************************
 FUNCTION: ASTROPCI_CLOSE()
 
 PURPOSE:  Entry point. Close a device from access.
 
 RETURNS:  Returns 0 always.
******************************************************************************/
static int astropci_close( struct inode *inode, struct file *file )
{
	long devnum = 0;

	astropci_printf( "( astropci_close ): aquiring spin lock ... " );
	spin_lock( &astropci_lock );
	astropci_printf( "done!\n" );

	devnum = iminor( inode );

	// Set the device state to closed
	devices[ devnum ].opened              = 0;
	devices[ devnum ].imageBufferPhysAddr = 0;
	devices[ devnum ].imageBufferVirtAddr = 0;

	astropci_printf( "( astropci_close ): devices[ %ld ].opened: %d\n", devnum, devices[ devnum ].opened );

	// Check if all devices are closed. If so, reset nextValidStartAddress
	for ( devnum = 0; devnum < MAX_DEV; devnum++ )
	{
 		if ( devices[ devnum ].opened ) { break; }
	}

	if ( devnum >= MAX_DEV )
	{
		nextValidStartAddress = 0;
	}

	spin_unlock( &astropci_lock );

	// Decrease the module usage count
	module_put( THIS_MODULE );

	return 0;
}

/******************************************************************************
 FUNCTION: ASTROPCI_IOCTL()
 
 PURPOSE:  Entry point. Control a character device.
 
 RETURNS:  Returns 0 for success, or the appropriate error number.
 
 NOTES:    The spinlocks have been removed because they shouldn't be used
           here since the functions used here can sleep. This will cause a
           processor to spin forever and deadlock when two PCI boards are
           active simultaneously. This is because the spin lock is global.
           A mutex (semaphore) can be used here, but it causes the load/unload
           process to result in WriteHCVR failure for some reason! Frankly,
           I don't think any locking is needed since each instance of the
           driver accesses different hardware and each instance is only
           opened by one program at a time.
******************************************************************************/
static long astropci_ioctl( struct file *file, unsigned int cmd, unsigned long arg )
{
	uint32_t reg   = 0;
	uint32_t reply = 0;
	int result     = 0;
	int devnum     = 0;
	int ctrlCode   = 0;

	devnum = iminor( file->f_dentry->d_inode );

	// Check that this device actually exists
	if ( devnum < 0 || devnum >= MAX_DEV || devices[ devnum ].has_irq == 0 )
	{
		result = -ENXIO;
	}

	else
	{
		if ( down_interruptible( &devices[ devnum ].sem ) )
		{
			astropci_printf( "( astropci_ioctl ): Failed to obtain SEMA4!\n" );
			return -ERESTARTSYS;
		}

		ctrlCode = EXCMD( cmd );

		lock_kernel();

		switch ( ctrlCode )
		{
			// -----------------------------------------------
			//  GET HCTR
			// -----------------------------------------------
        	case ASTROPCI_GET_HCTR:
			{
				reg = Read_HCTR( devnum );

				if ( put_user( reg, ( uint32_t * ) arg ) )
				{
					result = -EFAULT;
				}
			} break;

 			// -----------------------------------------------
			//  GET PROGRESS
			//  GET FRAMES READ
			//  GET CR PROGRESS
			// -----------------------------------------------
        	case ASTROPCI_GET_PROGRESS:
			case ASTROPCI_GET_FRAMES_READ:
			{
				uint32_t progress = 0;
				uint32_t upper    = 0;
				uint32_t lower    = 0;

				// Ask the PCI board for the current value
				if ( ctrlCode == ASTROPCI_GET_PROGRESS )
				{
                	result = Write_HCVR( devnum,
							     ( uint32_t )READ_PCI_IMAGE_ADDR );
				}

				else if ( ctrlCode == ASTROPCI_GET_FRAMES_READ )
				{
                	result = Write_HCVR( devnum,
							     ( uint32_t )READ_NUMBER_OF_FRAMES_READ );
				}

                		// Read the current image address
				if ( result == 0 )
				{
                	if ( astropci_check_dsp_output_fifo( devnum ) == OUTPUT_FIFO_OK_MASK )
					{
                		lower = Read_REPLY_BUFFER_16( devnum );
                		upper = Read_REPLY_BUFFER_16( devnum );
                		progress = ( ( upper << 16 ) | lower );
	            	}
                	else
                	{
                 		result = -EFAULT;
					}
				}

		       	if ( put_user( progress, ( uint32_t * )arg ) )
				{
		        	result = -EFAULT;
				}

			} break;

			// -----------------------------------------------
			//  GET HSTR
			// -----------------------------------------------
        	case ASTROPCI_GET_HSTR:
			{
                reg = Read_HSTR( devnum );
                
				if ( put_user( reg, ( uint32_t * )arg ) )
 				{
					result = -EFAULT;
				}
			} break;

			// -----------------------------------------------
			//  GET DMA ADDR
			// -----------------------------------------------
			case ASTROPCI_GET_DMA_ADDR:
			{
				uint32_t u32DmaAddr = devices[ devnum ].imageBufferPhysAddr;

				if ( put_user( u32DmaAddr, ( uint32_t * ) arg ) )
				{
					result = -EFAULT;
				}
			} break;

			// -----------------------------------------------
			//  ASTROPCI_GET_DMA_SIZE
			// -----------------------------------------------
			case ASTROPCI_GET_DMA_SIZE:
			{
				uint32_t u32BufSize = devices[ devnum ].imageBufferSize;

				if ( put_user( u32BufSize, ( uint32_t * ) arg ) )
				{
					result = -EFAULT;
				}
			} break;

			// -----------------------------------------------
			//  SET HCTR
			// -----------------------------------------------
			case ASTROPCI_SET_HCTR:
			{
				if ( get_user( reg, ( uint32_t * ) arg ) )
				{
					result = -EFAULT;
				}
				else
				{
					Write_HCTR( devnum, reg );
				}
			} break;

			// -----------------------------------------------
			//  SET HCVR
			// -----------------------------------------------
			case ASTROPCI_SET_HCVR:
			{
				uint32_t HcvrCode;

				if ( get_user( HcvrCode, ( uint32_t * ) arg ) )
				{
					result = -EFAULT;
				}
				else
				{
					// Clear the status bits if command not ABORT_READOUT. 
					// The pci board can't handle maskable commands 
					// (0xXX) during readout.
					if ( HcvrCode != ABORT_READOUT )
					{
						result = Write_HCVR( devnum, ( uint32_t )CLEAR_REPLY_FLAGS );
					}

					// Pass the command to the PCI board
					if ( result == 0 )
					{
						result = Write_HCVR( devnum, HcvrCode );
					}

					if ( result == 0 )
					{
						// Return reply
						reply = astropci_check_reply_flags( devnum );

						if ( reply == RDR )
						{
							// Flush the reply buffer
							astropci_flush_reply_buffer( devnum );

							// Go read some data
							result = Write_HCVR( devnum, ( uint32_t )READ_REPLY_VALUE );

							if ( result == 0 )
							{
								if ( astropci_check_dsp_output_fifo( devnum ) )
								{
									reply = Read_REPLY_BUFFER_32( devnum );
								}
 								else
				 				{
 									result = -EFAULT;
								}
							}
						}
					}
				}

				// A value must be returned to the user,
				// so do not protect it.
				if ( put_user( reply, ( uint32_t * ) arg ) )
				{
					result = -EFAULT;
				}
			} break;

			// -----------------------------------------------
			//  SET HCVR DATA
			// -----------------------------------------------
			case ASTROPCI_HCVR_DATA:
			{
				uint32_t Cmd_data;

				if ( get_user( Cmd_data, ( uint32_t * ) arg ) )
 				{
					result = -EFAULT;
				}
				else
				{
					if ( astropci_check_dsp_input_fifo( devnum ) )
					{
						Write_CMD_DATA_32( devnum, Cmd_data );
					}
					else
					{
						result = -EIO;
					}
				}
			} break;

			// -----------------------------------------------
			//  SEND COMMAND
			// -----------------------------------------------
			case ASTROPCI_COMMAND:
			{
				uint32_t Cmd_data[ 6 ] = { 0, 0, 0, 0, 0, 0 };
				uint32_t currStatus    = 0;
				int numberOfParams     = 0;
				int i                  = 0;

				if ( copy_from_user( Cmd_data, ( uint32_t * ) arg, sizeof( Cmd_data ) ) )
 				{
					result = -EFAULT;
				}

				else
				{
					// Check that the command isn't maskable and that
					// we're currently not in readout.
					currStatus = ( Read_HSTR( devnum ) & HTF_BIT_MASK ) >> 3;

					if ( ( Cmd_data[ 1 ] & 0x8000 ) == 0 && currStatus == READOUT_STATUS )
					{
						result = -EIO;
					}

					// Clear the status bits
					if ( result == 0 )
					{
						result = Write_HCVR( devnum, ( uint32_t )CLEAR_REPLY_FLAGS );
					}
                        
					// Wait for the FIFO to be empty.
					if ( result == 0 )
					{
						if ( !astropci_check_dsp_input_fifo( devnum ) )
						{
							result = -EIO;
						}
					}
						
					if ( result == 0 )
					{
						// Get the number of command parameters.
						numberOfParams = Cmd_data[ 0 ] & 0x000000FF;

						if ( numberOfParams > CMD_MAX )
						{
							astropci_printf( "(astropci_ioctl): Incorrect number of command parameters!\n" );
							result = -EFAULT;
						}
						else
						{
							// All is well, so write rest of the data.
							for ( i = 0; i < numberOfParams; i++ )
							{
								Write_CMD_DATA_32( devnum, Cmd_data[ i ] );
								//astropci_printf( "astropci: CMD[ %d ]: 0x%X\n", i, Cmd_data[ i ] );
							}

							// Tell the PCI board to do a WRITE_COMMAND vector command
							result = Write_HCVR( devnum, ( uint32_t )WRITE_COMMAND );
							//astropci_printf( "astropci: WRITE_COMMAND reply: 0x%X\n", result );
						}
					}

					if ( result == 0 )
					{
						// Set the reply
						reply =  astropci_check_reply_flags( devnum );

						if ( reply == RDR )
						{
							// Flush the reply buffer
							astropci_flush_reply_buffer( devnum );

							// Go read some data
							result = Write_HCVR( devnum, ( uint32_t )READ_REPLY_VALUE );

							if ( result == 0 )
							{
								if ( astropci_check_dsp_output_fifo( devnum ) )
								{
									reply = Read_REPLY_BUFFER_32( devnum );
								}
								else
								{
									result = -EFAULT;
								}
							}
						}
					}
				}

				// Return reply
				Cmd_data[ 0 ] = reply;

				if ( copy_to_user( ( uint32_t * ) arg, Cmd_data, sizeof( Cmd_data ) ) )
				{
					result = -EFAULT;
				}
			} break;

			// -----------------------------------------------
			//  PCI DOWNLOAD
			// -----------------------------------------------
			case ASTROPCI_PCI_DOWNLOAD:
			{
				// This vector command is here because
				// it expects NO reply.
				result = Write_HCVR( devnum,
						     ( uint32_t )PCI_DOWNLOAD );
			} break;

			// -----------------------------------------------
			//  PCI DOWNLOAD WAIT
			// -----------------------------------------------
			case ASTROPCI_PCI_DOWNLOAD_WAIT:
			{
				reply = astropci_check_reply_flags( devnum );

				if ( put_user( reply, ( uint32_t * )arg ) )
				{
					result = -EFAULT;
				}
			} break;

			// +-----------------------------------------------+
			// |  ASTROPCI_GET_CONFIG_BYTE                     |
        		// +-----------------------------------------------+
			case ASTROPCI_GET_CONFIG_BYTE:
			{
				uint32_t u32CfgReg = 0;
				uint8_t  u8Value   = 0;

        		result = get_user( u32CfgReg, ( uint32_t * )arg );

				if ( !IS_ERR_VALUE( result ) )
				{
					result = pci_read_config_byte( devices[ devnum ].pdev,
												   u32CfgReg,
												   &u8Value );
				}

				if ( !IS_ERR_VALUE( result ) )
				{
					result = put_user( ( uint32_t )u8Value,
									   ( uint32_t * )arg );
				}
			}
			break;

			// +-----------------------------------------------+
			// |  ASTROPCI_GET_CONFIG_WORD                     |
        		// +-----------------------------------------------+
			case ASTROPCI_GET_CONFIG_WORD:
			{
				uint32_t u32CfgReg = 0;
				uint16_t u16Value  = 0;

				result = get_user( u32CfgReg,
        						   ( uint32_t * )arg );

				if ( !IS_ERR_VALUE( result ) )
				{
					result = pci_read_config_word( devices[ devnum ].pdev,
												   u32CfgReg,
												   &u16Value );
				}

				if ( !IS_ERR_VALUE( result ) )
				{
					result = put_user( ( uint32_t )u16Value,
									   ( uint32_t * )arg );
				}
			}
			break;

			// +-----------------------------------------------+
			// |  ASTROPCI_GET_CONFIG_DWORD                    |
        		// +-----------------------------------------------+
			case ASTROPCI_GET_CONFIG_DWORD:
			{
				uint32_t u32CfgReg = 0;
				uint32_t u32Value  = 0;

				result = get_user( u32CfgReg, ( uint32_t * )arg );

				if ( !IS_ERR_VALUE( result ) )
				{
					result = pci_read_config_dword( devices[ devnum ].pdev,
													u32CfgReg,
													&u32Value );
				}

				if ( !IS_ERR_VALUE( result ) )
				{
					result = put_user( u32Value,
									   ( uint32_t * )arg );
				}
			}
			break;

			// +-----------------------------------------------+
			// |  ASTROPCI_SET_CONFIG_BYTE                     |
			// +-----------------------------------------------+
			// |  Parameter definition:                        |
			// |  --------------------                         |
			// |  Index 0: Configuration space byte offset     |
			// |  Index 1: Value to write                      |
        		// +-----------------------------------------------+
			case ASTROPCI_SET_CONFIG_BYTE:
			{
				uint32_t a32Param[ 2 ] = { 0, 0 };

				result = copy_from_user( a32Param,
										 ( uint32_t * )arg,
										 sizeof( a32Param ) );

				if ( !IS_ERR_VALUE( result ) )
				{
					result =
						pci_write_config_byte(
								devices[ devnum ].pdev,
								a32Param[ CFG_OFFSET ],
								( uint8_t )a32Param[ CFG_VALUE ] );
				}
			}
			break;

			// +-----------------------------------------------+
			// |  ASTROPCI_SET_CONFIG_WORD                     |
			// +-----------------------------------------------+
			// |  Parameter definition:                        |
			// |  --------------------                         |
			// |  Index 0: Configuration space byte offset     |
			// |  Index 1: Value to write                      |
			// +-----------------------------------------------+
			case ASTROPCI_SET_CONFIG_WORD:
			{
				uint32_t a32Param[ 2 ] = { 0, 0 };

				result = copy_from_user( a32Param,
										 ( uint32_t * )arg,
										 sizeof( a32Param ) );

				if ( !IS_ERR_VALUE( result ) )
				{
					result =
						pci_write_config_word(
								devices[ devnum ].pdev,
								a32Param[ CFG_OFFSET ],
								( uint16_t )a32Param[ CFG_VALUE ] );
				}
			}
			break;

			// +-----------------------------------------------+
			// |  ASTROPCI_SET_CONFIG_DWORD                    |
			// +-----------------------------------------------+
			// |  Parameter definition:                        |
			// |  --------------------                         |
			// |  Index 0: Configuration space byte offset     |
			// |  Index 1: Value to write                      |
			// +-----------------------------------------------+
			case ASTROPCI_SET_CONFIG_DWORD:
			{
				uint32_t a32Param[ 2 ] = { 0, 0 };

				result = copy_from_user( a32Param,
										 ( uint32_t * )arg,
										 sizeof( a32Param ) );

				if ( !IS_ERR_VALUE( result ) )
				{
					result =
						pci_write_config_dword(
									devices[ devnum ].pdev,
									a32Param[ CFG_OFFSET ],
									a32Param[ CFG_VALUE ] );
				}
			}
			break;

			// -----------------------------------------------
			//  DEFAULT
			// -----------------------------------------------
			default:
			{
				result = -EINVAL;
			} break;
		}  // switch

		unlock_kernel();

		up( &devices[ devnum ].sem );

	}  // end else

	return ( long )result;
}

/******************************************************************************
 FUNCTION: ASTROPCI_ISR()
 
 PURPOSE:  Entry point. Interrupt handler.
 *****************************************************************************/
static irqreturn_t astropci_ISR( int irq, void *dev_id )
{
	uint32_t int_flag = 0;
	long*    pDevNum  = 0;
	int      devnum   = 0;

	// NOTE: the dev_id value cannot be null for shared interrupts,
	// so it was incremented when initialized. Thus, it must be
	// decremented here.
	pDevNum = ( long * )dev_id;
	devnum   = *pDevNum - 1;

//	#ifdef DEBUG_ON
//	astropci_printf( "(astropci_ISR): device id: %d  dev_id param: %d\n", devnum, ( int )dev_id );
//	#endif

	if ( ( devnum >= 0 ) && ( devnum < MAX_DEV ) && ( devices[ devnum ].has_irq == 1 ) )
	{
		int_flag = Read_HSTR( devnum );
	}

//	astropci_printf( "(astropci_ISR): dev: %d, has_irq: %d, int_flag: 0x%X\n",
//				devnum, devices[ devnum ].has_irq, int_flag );

	// If no devices match the interrupting device, then exit
	if ( !( int_flag & DMA_INTERRUPTING ) )
	{
//		#ifdef DEBUG_ON
//		astropci_printf( "(astropci_ISR): couldn't find interrupt device (%d)\n",
//					devnum );
//		#endif
		return -IRQ_NONE;
	}

	// Clear the interrupt, no questions asked
//	#ifdef DEBUG_ON
//	astropci_printf( "(astropci_ISR): clearing Interrupts!\n" );
//	#endif

	Write_HCVR( devnum, ( uint32_t )CLEAR_INTERRUPT );

	return 0;
}

/******************************************************************************
 FUNCTION: ASTROPCI_SET_BUFFER_ADDRESSES
 
 PURPOSE:  Pass the DMA kernel buffer address to the DSP
 
 RETURNS:  0 on success, -ENXIO on failure.
******************************************************************************/
static int astropci_set_buffer_addresses( int devnum )
{
	uint32_t phys_address	= 0;
	int result		= 0;
	int reply		= 0;

	// Clear the reply flags - Oct 23, 2008
	Write_HCVR( devnum, ( uint32_t )CLEAR_REPLY_FLAGS );

	// Pass the DMA kernel buffer address to the DSP
	if ( astropci_check_dsp_input_fifo( devnum ) )
	{
		phys_address = devices[ devnum ].imageBufferPhysAddr;
		Write_CMD_DATA_16( devnum, ( uint16_t )( phys_address & 0x0000FFFF ) );
		Write_CMD_DATA_16( devnum, ( uint16_t )( ( phys_address & 0xFFFF0000 ) >> 16 ) );

		astropci_printf(
			"(astropci_set_buffer_addresses): DMA buffer address ( 0x%X )set on PCI board\n",
			phys_address );
 	}
	else
	{
		astropci_printf( "(astropci_set_buffer_addresses): Timeout while \
					setting DMA buffer address\n" );
		result = -ENXIO;
	}

	if ( result == 0 )
	{
		result = Write_HCVR( devnum, ( uint32_t )WRITE_PCI_ADDRESS );

 		// Check the reply
		if ( result == 0 )
		{
			reply = astropci_check_reply_flags( devnum );

			if ( reply != DON )
			{
				astropci_printf(
					"(astropci_set_buffer_addresses): WRITE_PCI_ADDRESS failed! Device: %d Reply: 0x%X\n",
					devnum, reply );

				result = -ENXIO;
			}
		}
	}

	return result;
}

/******************************************************************************
 FUNCTION: ASTROPCI_FLUSH_REPLY_BUFFER
 
 PURPOSE:  Utility function to clear DSP reply buffer and driver 
           internal value.
 RETURNS:  Returns 0 if successful. Non-zero otherwise.
******************************************************************************/
static int astropci_flush_reply_buffer( int devnum )
{
	uint32_t status = 0;
	int reply       = 0;
	int i           = 0;

	// Flush the reply buffer FIFO on the PCI DSP.
	// 6 is the number of 24 bit words the FIFO can hold.
	for ( i = 0; i < 6; i++ )
	{
		status = Read_HSTR( devnum );

		if ( ( status & OUTPUT_FIFO_OK_MASK ) == OUTPUT_FIFO_OK_MASK )
		{
			reply = Read_REPLY_BUFFER_32( devnum );
		}
		else
		{
			break;
		}
	}

	return reply;
}

/******************************************************************************
 FUNCTION: ASTROPCI_CHECK_REPLY_FLAGS
 
 PURPOSE:  Check the current PCI DSP status. Uses HSTR HTF bits 3,4,5.
 
 RETURNS:  Returns DON if HTF bits are a 1 and command successfully completed.
           Returns RDR if HTF bits are a 2 and a reply needs to be read.
           Returns ERR if HTF bits are a 3 and command failed.
           Returns SYR if HTF bits are a 4 and a system reset occurred.

 NOTES:    This function must be called after sending a command to the PCI
           board or controller.
******************************************************************************/
static int astropci_check_reply_flags( int devnum )
{
	uint32_t status = 0;
	int reply       = TIMEOUT;

	do {
		status = astropci_wait_for_condition( devnum, CHECK_REPLY );

		if ( status == DONE_STATUS )
		{
			reply = DON;
		}

		else if ( status == READ_REPLY_STATUS )
		{
			reply = RDR;
		}

		else if ( status == ERROR_STATUS )
		{
			reply = ERR;
		}

		else if ( status == SYSTEM_RESET_STATUS )
		{
			reply = SYR;
		}

		else if ( status == READOUT_STATUS )
		{
			reply = READOUT;
		}

		// Clear the status bits if not in READOUT
		if ( reply != READOUT )
		{
 			Write_HCVR( devnum, ( uint32_t )CLEAR_REPLY_FLAGS );
		}

	} while ( status == BUSY_STATUS );

	return reply;
}

/******************************************************************************
 FUNCTION: ASTROPCI_CHECK_DSP_INPUT_FIFO
 
 PURPOSE:  Check that the DSP input buffer (fifo) is not full.
 
 RETURNS:  Returns INPUT_FIFO_OK_MASK if HSTR bit 1 is set and buffer is
           available for input.

           Returns 0 if HSTR bit 1 is unset and buffer is unavailable 
           for input.

 NOTES:    This function must be called before writing to any register in the
           astropci_regs structure. Otherwise, data may be overwritten in the
           DSP input buffer, since the DSP cannot keep up with the input rate.
           This function will exit after "max_tries". This will help prevent
           the system from hanging in case the PCI DSP hangs.
******************************************************************************/
static int astropci_check_dsp_input_fifo( int devnum )
{
	return astropci_wait_for_condition( devnum, INPUT_FIFO );
}

/******************************************************************************
 FUNCTION: ASTROPCI_CHECK_DSP_OUTPUT_FIFO
 
 PURPOSE:  Check that the DSP output buffer (fifo) has data.
 
 RETURNS:  Returns OUTPUT_FIFO_OK_MASK if HSTR bit 1 is set and buffer is
           available for output?
           
 NOTES:    Please insert documentation here... 
******************************************************************************/
static int astropci_check_dsp_output_fifo( int devnum )
{
	return astropci_wait_for_condition( devnum, OUTPUT_FIFO );
}

/******************************************************************************
 FUNCTION: ASTROPCI_WAIT_FOR_CONDITION
 
 PURPOSE:  Waits for a particular state of the HSTR register. The
           condition can be INPUT_FIFO, OUTPUT_FIFO, or CHECK_REPLY.
 
 RETURNS:  0 on timeout, else a masked copy of the value of HSTR.
           
 NOTES:    The condition is first tested, and if satisfied, the routine
           returns immediately. Else, up to BUSY_MAX_WAIT microseconds
           are spent polling every BUSY_WAIT_DELAY microseconds. If
           the condition is still unsatisfied, up to SLEEP_MAX_WAIT
           microseconds total time will be spent, sleeping in intervals
           of SLEEP_WAIT_DELAY microseconds.

           So, this routine will busy wait for at most BUSY_MAX_WAIT 
           microseconds, and is guaranteed to return within SLEEP_MAX_WAIT
           microseconds (provided SLEEP_MAX_WAIT is greater than 
           BUSY_MAX_WAIT) plus or minus a jiffy.

           BUSY_WAIT_DELAY should be choosen so as not to overly
           tax the PCI card.

           BUSY_MAX_WAIT should be short enough not to cause unacceptable
           non-responsiveness of the computer, but long enough to cope
           with typical hardware delays.

           SLEEP_WAIT_DELAY should be at least a jiffy or three.

           SLEEP_MAX_WAIT should be the longest time before we clearly
           have a timeout problem.
******************************************************************************/
static uint32_t astropci_wait_for_condition( int devnum, int condition_to_wait_for )
{
	uint32_t status      = 0;
	uint32_t elapsedTime = 0L;
	uint32_t sleepDelay  = 0L;
	struct timeval initial, now;

	// Set the sleep delay (in jiffies)
	sleepDelay = ( SLEEP_WAIT_DELAY * HZ ) / 1000000L;
	if ( sleepDelay < 1 ) { sleepDelay = 1; }
 
	// Get the current time
	do_gettimeofday( &initial );

	// Loop for the specified number of usec
	while ( elapsedTime < 10 * SLEEP_MAX_WAIT )	// sds - Oct 23, 2008
	{
		status = Read_HSTR( devnum );

		switch ( condition_to_wait_for )
		{
			case INPUT_FIFO:
			{
                        	status &= INPUT_FIFO_OK_MASK;
                        }
			break;

			case OUTPUT_FIFO:
			{
				status &= OUTPUT_FIFO_OK_MASK;
			}
                       	break;

			case CHECK_REPLY:
			{
                        	status = ( status & HTF_BIT_MASK ) >> 3;
			}
	               	break;
		}

		if ( status > 0 )
			break;

		// Get the current time and the elapsed time (usec)
		do_gettimeofday( &now );

		elapsedTime = ( now.tv_sec  - initial.tv_sec ) * 1000000L
				  + ( now.tv_usec - initial.tv_usec );

		if ( elapsedTime < BUSY_MAX_WAIT )
		{
			// Busy wait delay
			udelay( BUSY_WAIT_DELAY );
 		}
		else
		{
			// Sleep delay
			current->state = TASK_UNINTERRUPTIBLE;
			schedule_timeout( sleepDelay );
		}
	}

	return status;
}

/******************************************************************************
 FUNCTION: ASTROPCI_MMAP
        
         - Brian Taylor contributed to the original version of this function.

 PURPOSE:  Map the image buffer from unused RAM.
 
 RETURNS:  Returns 0 if memory was mapped, else a negative error code.
 
 NOTES:    For this function to work, the user must specify enough buffer
           space by passing "mem=xxxM" to the kernel as a boot parameter. 
           If you have 128M RAM and want a 28M image buffer, then use: 
           "mem=100M".

		From: "Eli Rykoff" <erykoff@umich.edu>:

		The bug is in the astropci_mmap() function, and applies to all
		versions of the astropci driver (RH9, FC2, FC4), when using at
		least 1GB of system ram.  It has to do with how the Linux kernel
		defines "low memory" and "high memory." (Which I didn't know
		before, but I sure do now). The Linux kernel can only directly
		address up to 915MB of memory (with usual stock compile options),
		and the rest of the system ram is the "high memory".  The problem
		comes in determining where the top of the system ram is.  In the
		current version of the driver, this is set as "__pa(high_memory)",
		which (as it turns out) is only equal to the top of the system ram
		if you have <915MB, and maxes out at 0x38000000.  If you have >= 1GB
		of ram then you can allocate this memory, but as soon as you try to
		use it you get problems. Unfortunately, I have found that the
		"right" way to do this is not very well documented (or documented
		at all, for that matter), but poking into mm.c and page.h in the
		Linux source reveals that the top of the memory is at the address
		num_physpages*PAGE_SIZE.  So, the fix is pretty simple:

		in astropci.c(FC4)/astropci_main.c(RH9), astropci_mmap():

            // Identify a suitable high memory area to map
            if ( nextValidStartAddress == 0 )
		{
			#ifdef CONFIG_HIGHMEM
                  nextValidStartAddress = (unsigned long) num_physpages * PAGE_SIZE;

			// this is the top of the Linux memory addresses
			#else
                  nextValidStartAddress = (unsigned long) __pa(high_memory);
			#endif
            }

		I tested this on both SL3 and FC4 (both systems with 2gb ram) and
		it works as it should.
******************************************************************************/
static int astropci_mmap( struct file *file, struct vm_area_struct *vma )
{
	unsigned int prot = 0;
	int devnum        = 0;
	int result        = 0;
	int i             = 0;

	#ifdef DEBUG_ON
	unsigned long *buffer;
	#endif

	spin_lock( &astropci_lock );

	devnum = ( long )file->private_data;

	// Sanity check, failure here should be impossible.
	if ( ( devnum < 0 ) || ( devnum >= MAX_DEV ) || ( !devices[ devnum ].opened ) )
	{
		astropci_printf( "(astropci_mmap): invalid device number -> %d\n", devnum );
		result = -EAGAIN;
	}

	else
	{
		astropci_printf( "(astropci_mmap): mapping memory for device %d\n", devnum );

		// Identify a suitable high memory area to map
		if ( nextValidStartAddress == 0 )
		{
			#ifdef CONFIG_HIGHMEM
			astropci_printf( "(astropci_mmap): configuring for CONFIG_HIGHMEM\n" );
	                nextValidStartAddress = ( unsigned long )num_physpages * PAGE_SIZE;
			#else
			// this is the top of the Linux memory addresses
			astropci_printf( "(astropci_mmap): configuring for use of __pa()\n" );
	                nextValidStartAddress = ( unsigned long )__pa( high_memory );
			#endif
		}

		// Save the physical address
		devices[ devnum ].imageBufferPhysAddr = nextValidStartAddress;
		devices[ devnum ].imageBufferSize = ( vma->vm_end - vma->vm_start );

		nextValidStartAddress = devices[ devnum ].imageBufferPhysAddr +
						devices[ devnum ].imageBufferSize + PAGE_SIZE;

		astropci_printf( "(astropci_mmap): board %d  buffer start: 0x%X  end: 0x%X size: %u\n",
				    devnum, devices[ devnum ].imageBufferPhysAddr, 
				    devices[ devnum ].imageBufferPhysAddr + devices[ devnum ].imageBufferSize,
				    devices[ devnum ].imageBufferSize );

		// Ensure that the memory will not be cached; see drivers/char/mem.c
		if ( boot_cpu_data.x86 > 3 )
		{
			prot = pgprot_val( vma->vm_page_prot ) | _PAGE_PCD | _PAGE_PWT;
			vma->vm_page_prot = __pgprot( prot );
		}
 
 		// Don't try to swap out physical pages
		vma->vm_flags |= VM_RESERVED;

		// Don't dump addresses that are not real memory to a core file
		vma->vm_flags |= VM_IO;

		// Remap the page range to see the high memory
		astropci_printf( "(astropci_mmap): Remapping page range for kernel 2.6.x\n" );
		result = remap_pfn_range( vma,
					  vma->vm_start,
				      	  ( devices[ devnum ].imageBufferPhysAddr >> PAGE_SHIFT ),
                                          ( vma->vm_end - vma->vm_start ),
                                          vma->vm_page_prot );

		if ( result != 0 )
		{
			astropci_printf( "(astropci_mmap): remap page range failed.\n" );
		}
		else
		{
        		astropci_printf( "(astropci_mmap): %u bytes mapped: 0x%lX - 0x%lX --> 0x%X - 0x%X\n",
					    devices[ devnum ].imageBufferSize, vma->vm_start, vma->vm_end,
					    devices[ devnum ].imageBufferPhysAddr, 
 					    devices[ devnum ].imageBufferPhysAddr + devices[ devnum ].imageBufferSize );

			// Save the virtual address, this seems to be what the driver
			// needs in order to access the image buffer.
			devices[ devnum ].imageBufferVirtAddr = vma->vm_start;

			astropci_printf( "(astropci_mmap): devices[ devnum ].imageBufferVirtAddr: 0x%lX\n",
					  devices[ devnum ].imageBufferVirtAddr );

			astropci_printf( "(astropci_mmap): devices[ devnum ].imageBufferVirtAddr sizeof: %d\n",
					  ( 8 * sizeof( devices[ devnum ].imageBufferVirtAddr ) ) );

			astropci_printf( "(astropci_mmap): devices[ devnum ].imageBufferVirtAddr sizeof: %d\n",
					  ( 8 * sizeof( vma->vm_start ) ) );

	#ifdef DEBUG_ON
			// Write some test values to the image buffer.
			astropci_printf( "(astropci_mmap): writing test values to image buffer\n" );
			buffer = ( unsigned long * )devices[ devnum ].imageBufferVirtAddr;

			if ( buffer != NULL )
			{
				for ( i=0; i<devices[ devnum ].imageBufferSize/sizeof( unsigned long ); i++ )
				{
					buffer[ i ] = 0xDEADFACE;
				}

//				buffer[ 0 ] = 0x11111111;
//				buffer[ 1 ] = 0x22222222;
//				buffer[ 2 ] = 0x33333333;
//				buffer[ 3 ] = 0x44444444;

				astropci_printf( "(astropci_mmap): buffer[0]: 0x%lX\n", buffer[ 0 ] );
				astropci_printf( "(astropci_mmap): buffer[1]: 0x%lX\n", buffer[ 1 ] );
				astropci_printf( "(astropci_mmap): buffer[2]: 0x%lX\n", buffer[ 2 ] );
				astropci_printf( "(astropci_mmap): buffer[3]: 0x%lX\n", buffer[ 3 ] );
			}
			else
			{
				astropci_printf( "(astropci_mmap): ERROR, failed to access image buffer!\n" );
			}
	#endif

        		// Assign the image buffers
        		result = astropci_set_buffer_addresses( devnum );

			if ( result != 0 )
			{
				astropci_printf( "(astropci_mmap): set PCI DMA buffer address failed.\n" );
			}
		}
	}

	// If there was a failure, set imageBufferPhysAddr to zero as a flag
	if ( ( result != 0 ) && ( devnum != MAX_DEV ) )
	{
		devices[ devnum ].imageBufferPhysAddr = 0;
		devices[ devnum ].imageBufferVirtAddr = 0;
	}

	spin_unlock( &astropci_lock );

	return result;
}

/******************************************************************************
 FUNCTION: READREGISTER_32

 PURPOSE:  This function wraps the basic "readl" function, which has a different
	   version for kernels after 2.4.x.
 
 RETURNS:  Returns the 32-bit value read from "addr".
******************************************************************************/
static unsigned int ReadRegister_32( long addr )
{
	ndelay( REGISTER_ACCESS_DELAY );

	return readl( ( unsigned long * )( addr ) );
}

/******************************************************************************
 FUNCTION: READREGISTER_16

 PURPOSE:  This function wraps the basic "readw" function, which has a different
	   version for kernels after 2.4.x.
 
 RETURNS:  Returns the 16-bit value read from "addr".
******************************************************************************/
static unsigned short ReadRegister_16( long addr )
{
	ndelay( REGISTER_ACCESS_DELAY );

        return readw( ( unsigned long * )( addr ) );
}

/******************************************************************************
 FUNCTION: WRITEREGISTER_32

 PURPOSE:  This function wraps the basic "writel" function, which has a different
	   version for kernels after 2.4.x. Writes a 32-bit value to "addr".
 
 RETURNS:  Returns nothing.
******************************************************************************/
static void WriteRegister_32( unsigned int value, long addr )
{
	ndelay( REGISTER_ACCESS_DELAY );

        writel( value, ( unsigned long * )( addr ) );
}

/******************************************************************************
 FUNCTION: WRITEREGISTER_16

 PURPOSE:  This function wraps the basic "writew" function, which has a different
	   version for kernels after 2.4.x. Writes a 16-bit value to "addr".
 
 RETURNS:  Returns nothing.
******************************************************************************/
static void WriteRegister_16( unsigned short value, long addr )
{
	ndelay( REGISTER_ACCESS_DELAY );

        writew( value, ( unsigned long * )( addr ) );
}

/******************************************************************************
 FUNCTION: READ_HCTR

 PURPOSE:  Reads a 32-bit value from the HCTR. Calls ReadRegister_32.
 
 RETURNS:  Returns the value read.
******************************************************************************/
static unsigned int Read_HCTR( int devnum )
{
	return ReadRegister_32( devices[ devnum ].ioaddr + HCTR );
}

/******************************************************************************
 FUNCTION: READ_HSTR

 PURPOSE:  Reads a 32-bit value from the HSTR. Calls ReadRegister_32.
 
 RETURNS:  Returns the value read.
******************************************************************************/
static unsigned int Read_HSTR( int devnum )
{
	return ReadRegister_32( devices[ devnum ].ioaddr + HSTR );
}

/******************************************************************************
 FUNCTION: READ_REPLY_BUFFER_32

 PURPOSE:  Reads a 32-bit value from the REPLY_BUFFER. Calls ReadRegister_32.
 
 RETURNS:  Returns the value read.
******************************************************************************/
static unsigned int Read_REPLY_BUFFER_32( int devnum )
{
	return ReadRegister_32( devices[ devnum ].ioaddr + REPLY_BUFFER );
}

/******************************************************************************
 FUNCTION: READ_REPLY_BUFFER_16

 PURPOSE:  Reads a 16-bit value from the REPLY_BUFFER. Calls ReadRegister_16.
 
 RETURNS:  Returns the value read.
******************************************************************************/
static unsigned short Read_REPLY_BUFFER_16( int devnum )
{
	return ReadRegister_16( devices[ devnum ].ioaddr + REPLY_BUFFER );
}

/******************************************************************************
 FUNCTION: WRITE_HCTR

 PURPOSE:  Writes a 32-bit value to the HCTR. Calls WriteRegister_32.
 
 RETURNS:  None
******************************************************************************/
static void Write_HCTR( int devnum, unsigned int regVal )
{
	WriteRegister_32( regVal, devices[ devnum ].ioaddr + HCTR );
}

/******************************************************************************
 FUNCTION: WRITE_CMD_DATA_32

 PURPOSE:  Writes a 32-bit value to the CMD_DATA register. Calls
	     WriteRegister_32.
 
 RETURNS:  None
******************************************************************************/
static void Write_CMD_DATA_32( int devnum, unsigned int regVal )
{
	WriteRegister_32( regVal, devices[ devnum ].ioaddr + CMD_DATA );
}

/******************************************************************************
 FUNCTION: WRITE_CMD_DATA_16

 PURPOSE:  Writes a 16-bit value to the CMD_DATA register. Calls
	     WriteRegister_16.
 
 RETURNS:  None
******************************************************************************/
static void Write_CMD_DATA_16( int devnum, unsigned short regVal )
{
	WriteRegister_16( regVal, devices[ devnum ].ioaddr + CMD_DATA );
}

/******************************************************************************
 FUNCTION: WRITE_HCVR

 PURPOSE:  Writes a 32-bit value to the HCVR. Checks that the HCVR register
	     bit 1 is not set, otherwise a command is still in the register.
	     Calls WriteRegister_32.
 
 RETURNS:  None
******************************************************************************/
static int Write_HCVR( int devnum, unsigned int regVal )
{
	unsigned int currentHcvrValue = 0;
	int i, status = -EIO;

	for ( i=0; i<100; i++ )
	{
		currentHcvrValue = ReadRegister_32( devices[ devnum ].ioaddr + HCVR );

		if ( ( currentHcvrValue & ( unsigned int )0x1 ) == 0 )
		{
			status = 0;
			break;
		}

		astropci_printf(
			"(Write_HCVR): HCVR not ready. Count: %d Value: 0x%X\n",
			i, currentHcvrValue );
	}

	if ( status == 0 )
	{
		WriteRegister_32( regVal, devices[ devnum ].ioaddr + HCVR );
	}

	return status;
}

MODULE_AUTHOR( "Scott Streit" );
MODULE_VERSION( "3.0" );
MODULE_DESCRIPTION( "ARC PCI Interface Driver" );
MODULE_SUPPORTED_DEVICE( "astropci" );

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
MODULE_LICENSE("GPL");
#endif
