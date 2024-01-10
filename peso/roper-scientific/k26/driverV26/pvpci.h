/**********************************************************
 * pvpci.h - the header file with the ioctl definitions.  *
 *							  *
 * The declarations here have to be in a header file, 	  *
 * because they need to be known both to the kernel 	  *
 * module (in pvpci.c) and the process calling ioctl 	  *
 **********************************************************/

#ifndef PVPCI_H
#define PVPCI_H

#include <linux/ioctl.h>

/* The major device number. */
#define MAJOR_NUM   	61

#define CAMERA_MINOR_MASK 0xf	/* This is used to mask off the Camera number in driver entry points */
#define OUR_PCI_INTERRUPT	0x00020000 /* Mask for our PCI Interrupt indicator */
#define MAIL_FOR_US 0x00f00000	/* Mask for determination of Mail for our driver */
#define MAX_CAMERAS	12
#define IMAGE_ORDER     2       // get 4 pages per block						 	
#define IMAGE_PAGES     4       // 2 ^ IMAGE_ORDER
#define TABLE_ORDER 	4    	// get 16 pages for scatter gather table
#define TABLE_PAGES 	16  	// 2 ^ TABLE_ORDER
#define TABLE_SIZE	8192	// scatter gather table for 128 meg images
				// (TABLE_PAGES * PAGE_SIZE) / 8
/*
 * IOCTL COMMANDS
 */

#define IOCTL_GET_MAXRETRIES   _IOW(MAJOR_NUM,10,int)
#define IOCTL_SET_MAXRETRIES   _IOR(MAJOR_NUM,11,int)
#define IOCTL_GET_MAXTIMEOUT   _IOW(MAJOR_NUM,12,int)
#define IOCTL_SET_MAXTIMEOUT   _IOR(MAJOR_NUM,13,int)
#define IOCTL_WRITE_READ       _IOWR(MAJOR_NUM,14,char *)

#define IOCTL_SET_IMAGE_ACTIVE _IOR(MAJOR_NUM,15,int)
#define IOCTL_SET_IMAGE_IDLE   _IOR(MAJOR_NUM,16,int)
#define IOCTL_GET_IMAGE_STATUS _IOW(MAJOR_NUM,17,int)
#define IOCTL_GET_IMAGE_GRANUL _IOW(MAJOR_NUM,22,int)

#define IOCTL_GET_PIXTIME      _IOW(MAJOR_NUM,18,int)
#define IOCTL_GET_DRIVER_VER   _IOW(MAJOR_NUM,19,int)

#define IOCTL_GET_VERBOSE_MODE _IOW(MAJOR_NUM,20,int)
#define IOCTL_SET_VERBOSE_MODE _IOR(MAJOR_NUM,21,int)

#define IOCTL_GET_INFO_LENGTH  _IOW(MAJOR_NUM,23,int)
#define IOCTL_GET_INFO_STR     _IOW(MAJOR_NUM,24,int)
#define IOCTL_GET_PCI_FW_REV   _IOW(MAJOR_NUM,30,int)
#define IOCTL_TEST	       _IOW(MAJOR_NUM,31,int)
#define IOCTL_LOAD_PCI_FLASH   _IOW(MAJOR_NUM,32,int)	

/* The name of the device file, as it will appear in /dev */
#define DEVICE_FILE_NAME "pvcam"

/* The name of the device, as it will appear in /proc/devices */
#define DEVICE_NAME "pvpci"


typedef unsigned char  	uns8,  *uns8_ptr;
typedef short          	int16, *int16_ptr;
typedef unsigned short 	uns16, *uns16_ptr;
typedef unsigned long  	uns32, *uns32_ptr;
typedef unsigned long 	DWORD;


typedef struct tagTE {
	uns32	pAddr;
	uns32	b_size;
} TABLE_ENTRY;


/* 
 * ioctl_wr_rd_type is used to pass information in and out of ioctl
 *   command class for this write-read instruction
 *   number of bytes to write: 1-32768
 *   location of data to send
 *   number of bytes to read:  0-32768 (0=no read)
 *   location to place date read
 */
typedef struct {
	uns8     c_class;
	uns16    write_bytes;
	uns8_ptr write_array;
	uns16    read_bytes;
	uns8_ptr read_array;
} ioctl_wr_rd_type, *ioctl_wr_rd_ptr;


/* 
 * ioctl_im_actv_type is used to pass information in and out of ioctl
 *   number bytes to collect (up to several meg)
 *   location to put data into
 */
typedef struct {
	uns32     totl_bytes;
	uns16_ptr pix_array;
} ioctl_im_actv_type, *ioctl_im_actv_ptr;


/* 
 * ioctl_im_stat_type used to pass information in and out of ioctl
 *   number bytes collected so far
 *   follows codes frm v00incl_.h 
 */
typedef struct {
	uns32  byte_cnt;
	int16  status;
} ioctl_im_stat_type, *ioctl_im_stat_ptr;


/*
 * ioctl_info_str_type used to pass the information string into and 
 * out of ioctl.
 *   number of bytes
 *   character string
 *   error code
 */
typedef struct {
	uns16	bytes;
	char	*info_str;
	int16	error_value;
}  ioctl_info_str_type, *ioctl_info_str_ptr;




/* 
 * Local camera data
 */
typedef struct {
	u_char       		state;
	u_char			xcmd_busy;
	u_char			xcmd_timedout;
	u_char			xfer_active;
	int         		maxtimeout;	/* msec */
	int          		image_status;
	int          		image_state;
	uns32        		image_total;
	uns32        		image_collected;
	uns8_ptr     		image_addr;
	uns8_ptr		rwbuf;
	uns32	   		intrStatus;
	struct pci_dev		*devp;	
	TABLE_ENTRY	    	*pTE;		/* Scatter Gather Table */
	uns32		    	table_size;
	uns32           	table_addr;
	uns32			used_entries;
	wait_queue_head_t	waitq;
	ioctl_wr_rd_ptr		wr_rd_datap;
	int			wr_rd_mode;
	u16			base_address[4];
	unsigned		irq;
	int			lock;		/* to handle race condition*/


//uns32 annacc;	/*test code*/
//uns32 annadmac;	/*test code*/
//uns32 annatotint; /*test code*/



}  cam_dev_type, *cam_dev_ptr;

typedef struct iovec_t *iovec_ptr;

/********************************************************
 * 			CONSTANTS			* 
 ********************************************************/
#if !defined FALSE
#define FALSE 0
#endif

#if !defined TRUE
#define TRUE  1
#endif

/********************************************************
 * 	Constants for Lock				*
 ********************************************************/
#define NO_LOCK		0
#define WRITE_LOCK	1
#define READ_LOCK	2


/********************************************************
 *			 MACROS				*
 ********************************************************/
#define RWBUF_NUM_PAGES		1					/* number of pages to read	*/
#define RWBUF_NUM_BYTES		(1<<RWBUF_NUM_PAGES) * PAGE_SIZE	/* num bytes to read based on 2	*/
									/* raised to the number of pages*/
#if !defined max
#define max(a,b)	((a) > (b) ? (a) : (b))
#endif

#if !defined min
#define min(a,b)	((a) < (b) ? (a) : (b))
#endif

#define SCRIPTBUFFERSIZE	4100

#define DDI_SUCCESS 	0
#define DDI_FAILURE	 	1

/********************************************************
 * 	    DRIVER STATES used by the CAMERA		*
 ********************************************************/
#define STATE_CLOSED    0
#define STATE_OPEN      1
#define PIXTIME       500

							
/************* STATUS CODES, COPIED (more or less) FROM V00INCL_.H ***********/

#define EXP_ST_IDLE 			1   /* status is idle */

#define EXP_ST_NO_DATA_A   		100 /* no data has arrived yet active state */
#define EXP_ST_NO_DATA_I   		101 /* no data has arrived yet idle state */

#define EXP_ST_COLLECT_OK_A 	102 /* collection is progressing correctly, active */
#define EXP_ST_COLLECT_DONE_I 	103 /* successfully acquired data & finish, idle */

#define EXP_ST_FIFO_OVER_A 		104 /* FIFO overflow */
#define EXP_ST_FIFO_OVER_I 		105 /* FIFO overflow idle state */

#define EXP_ST_EXTRA_DATA_I 	107 /* successfully acquired data & finished, but */
                               		/* there was more data than expected, idle */
#define EXP_ST_NO_ACK_A    		108 /* camera didn't acknowledge last  */
#define EXP_ST_NO_ACK_I    		109 /* request for image data, idle state */

#define EXP_ST_XFER_ERR_A  		110 /* last transmission from the camera */
#define EXP_ST_XFER_ERR_I  		111 /* was garbled, idle state */

#define EXP_ST_UNKNOWN     		113 /* we don't know what state it's in */

#define EXP_ST_MISSING_DATA_I 	115 /* collection done, less data than expected */

#define EXP_ST_EMPTY_DATA_I  	117 /* collection done, there was no data, idle*/


/******************************************************** 
 * Note - i						*
 * all PCI board access use (long *) data references,	*
 * therefore all S5933 offsets are divided by 4...	*
 ********************************************************/

#define OUTMAILBOX2		0x04
#define LATENCY			0x0D
#define MAILBOX1		0x10
#define MAILBOX2		0x14
#define MAILBOX3		0x18
#define MAILBOX4		0x1c
#define MAILSTATUS		0x34
#define INTCSR			0x38
#define MASTERCSR		0x3c
#define IOBASE			0x00

#define INT_MAILBOX1		0x20321000 // was 0x21321000
#define INT_MAILBOX2		0x20321400 // was 0x21321400
#define INT_DISABLE		0x20320000 // was 0x21320000
#define MASTERSET  		0x00009300
#define MASTERRESET     	0x0E009300
#define ABORT_EXP		0xDEADDEAD

#define PMPCI_VENDOR_ID		0x10E8
#define PMPCI_DEVICE_ID 	0x81e6

#define DRIVER_VERSION    	0x0400 	/* high byte:major ver.  nibbles: minor,triv*/
#define DEFAULT_TIMOUT     	2000   	/* timeout(msec) for reading data */
#define MIN_XFER_SIZE     	4
#define MAX_XFER_SIZE     	0x2000000     /* 32Mb */
#define IDLE_STATE  		0
#define ACTIVE_STATE  		1

#define LOAD_PCIFLASH  'L'	// Definition for PCI Firmware Load Command
#define LOAD_CAMBOOT   'B'
#define LOAD_CAMEEPROM 'E'
#define LOAD_CAMFLASH  'F'
#define RESET_INTERFACE 'X'	// Definition for Interface Reset Command

#endif





