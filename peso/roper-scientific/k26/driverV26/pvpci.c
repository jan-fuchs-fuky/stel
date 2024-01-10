/********************************************************************************
 *										*
 * pvpci.c 									*
 * 										*
 * Linux PVCAM PCI Device Driver						*
 *										*
 * Copyright (C) 1999 - 2004  Roper Scientific, Inc.				*
 *										*
 ********************************************************************************/
/********************************************************************************
 * 										*
 * 	NOTICE									*
 * 										*
 * 	This driver has been adapted for Red Hat Linux 9.0			*
 * 	using kernel 2.4.X.X and is not certified for use			*
 * 	with ANY earlier kernels.						*			
 * 										*
 ********************************************************************************/
/* Driver Mods for Red Hat 9.0 made by Glenn Witerski of IntrNetMethods Inc.    */
/* Driver Mods for Red Hat 9.0 made by Glenn Witerski of IntrNetMethods Inc.    */
/* Driver Mods for RH 9.0 for Firmware Rev 20 support                           */
/* Made by Glenn Witerski of IntrnetMethods Inc,                                */
/* July 15 2004                                                                 */
                                                                                                                         



/* Include the general kernel module support headers */
//#include <linux/config.h>
                                                                                                                         
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/autoconf.h>
#include <linux/pci.h>
#include <linux/fs.h>	
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/uaccess.h>

/* Check for Modern Versions-If so then add the modversions include file*/
#if CONFIG_MODERNVERSIONS==1
    #define MODVERSIONS
    #include <linux/modversions.h>
#endif

MODULE_LICENSE("GPL");


#ifndef KERNEL_VERSION
    #define KERNEL_VERSION(a,b,c) ((a)*65536+(b)*256+(c))
#endif

/*	 Check if we are kernel 2.2 or later - If so then include the new	*/
/*	 uaccess file								*/


/********************************************************************************/
/* Include our own Module Include file - This file contains many definitions	*/
/* Specific to our hardware as well as some items that are somewhat general	*/
/* in nature - But we felt that we needed control of those items since our	*/
/* PCI hardware varies slightly from most other DMA systems (See Note Below)	*/
/********************************************************************************/

#include "pvpci.h"



/********************************************************************************
 * NOTE: The PCI card for newer camera systems use a slightly different DMA	*
 * Protocol than most DMA systems - Let me explain "briefly"...			*
 * 										*
 * 								 		*
 * The PCI card that we are talking to is using the AMCC 5933 PCI Controller	*
 * Many systems use this controller - We are using it as a Bus Master DMA	*
 * device.  HOWEVER - Most Bus Master DMA devices usually have the Scatter	*
 * Gather table loaded into the PCI card and then the DMA controller uses	*
 * the local copy of the table in mapping and doing the DNA transfers.		*
 * 								 		*
 * We could not commit to a Maximum size for a transfer at the time of		*
 * design of the board (Remember the statement "We will never need more than	*
 * 640K of Main Memory" - Ever!) - Yep - you guessed it - We keep the S/G	*
 * table on the Host/PC side of the bus where the S/G table can be grown	*
 * as needed.  Instead of writing the S/G table to the PCI card - We simply	*
 * write the starting address of the table to the PCI card and the firmware	*
 * on the PCI takes this address and stores it for local use and starts the	*
 * DMA process immediately after getting the start command from the user.	*
 * Since the PCI card is a true Bus Master and it has the starting		*
 * address of the S/G table in PC memory - All it has to do is fire a Bus	*
 * Master transfer of 8 bytes when it wants to read an entry in the S/G		*
 * table - 4 bytes for Page Address and 4 bytes for block size.			*
 * It then has the information necessary to do a Bus Naster transfer the	*
 * other direction to place the pixel data into PC memory.  The PCI card	*
 * then increments the S/G table address by 8 bytes and does the whole thing	*
 * again until it is finished.  This is a simplified explanation of the		*
 * process but basically correct in nature. A Null terminated entry in the	*
 * S/G Table tells the PCI firmware when to stop DMA Bursts.			* 
 * 						 				*
 * Thanks to Dave Dalton for his insight for the future of this design.		*
 * ******************************************************************************/



/********************************************************************************
 * Driver entry points								*
 ********************************************************************************/

static int device_open(struct inode *inode,
                       struct file *filp);                                     /* This is run when the device is opened */

static int device_release(struct inode *inode,
                          struct file *file);                                  /* This is run when the device is closed */

/* This is NOT used in our driver but we must support it	*/
static ssize_t device_read(struct file *file,
                           char __user *buffer,                                       /* The buffer to fill with data		*/
                           size_t length,                                      /* The length of the buffer		*/
                           loff_t *offset);                                    /* Our offset in the file		*/

/* This is NOT used in our driver but we must support it	*/
static ssize_t device_write(struct file *file,    
                            const char __user *buffer,                                /* The buffer				*/
                            size_t length,                                     /* The length of the buffer		*/
                            loff_t *offset);                                   /* Our offset in the file		*/

/* This is the call that does most of the work in our driver	*/
static int device_ioctl(struct inode *inode,
                        struct file *filp,
                        unsigned int ioctl_num,                                /* The number of the ioctl		*/
                        unsigned long ioctl_param);                            /* The parameter to it		*/

/********************************************************************************
 *This structure will hold the functions to be called				*
 * when a process does something to the device we				*
 * created. Since a pointer to this structure is kept in			*
 * the devices table, it can't be local to					*
 * pvpci_init_module. NULL is for unimplemented functions.				*
 ********************************************************************************/

struct file_operations Fops = {
	.owner =	THIS_MODULE,
	.llseek=	NULL,
	.read=		device_read,
	.write=		device_write,
	.ioctl=		device_ioctl,
	.open=		device_open,
	.release=	device_release,
};



/********************************************************************************
 * 										*
 *		local function definitions					*
 *										*
 ********************************************************************************/
/* These are all functions that we use within the driver to manipulate the PCI	*
 * hardware									*
 ********************************************************************************/

int   pvpci_write_read(struct file *filp, ioctl_wr_rd_ptr ioclt_ptr);
int   pvpci_create_buffer(cam_dev_ptr cam, unsigned long buf_size);
void  pvpci_free_buffer(cam_dev_ptr cam);
void  pvpci_DisableIntr(cam_dev_ptr camera);
void  pvpci_EnableIntrMB1(cam_dev_ptr camera);
void  pvpci_EnableIntrMB2(cam_dev_ptr camera);
void  pvpci_ResetMBFlags(cam_dev_ptr camera);
long  pvpci_MailStatus(cam_dev_ptr camera);
void  pvpci_AbortExp(cam_dev_ptr camera);
static irqreturn_t pvpci_intr(int irq, void *dev_id, struct pt_regs *regs);
int loadPCIflash (struct file *filp, ioctl_wr_rd_ptr ioctl_ptr);


/* This is an array of "Camera" structures that is used for all of the Per Camera data */
static cam_dev_type cam[MAX_CAMERAS];

/* This is the internal count of number of cameras */
static int camcnt = 0;
//static int pairingvar = 0;	/*test code*/

//static uns32 ccitrcnt=0;		/*test code*/
//static uns32 tcitrcnt=0;		/*test code*/




/********************************************************************************/
/*										*/
/*				FUNCTIONS					*/
/*										*/
/********************************************************************************
 *										*
 * This function is called whenever a process attempts				*
 * to open the device file							*
 * 										*
 ********************************************************************************/
static int device_open(struct inode *inode, struct file *filp)
{
    int cn;

    cn = inode->i_rdev & CAMERA_MINOR_MASK;                                    /* Mask off all but the camera number	*/

    cam[cn].xcmd_busy = FALSE;
    cam[cn].xcmd_timedout = FALSE;
    cam[cn].xfer_active = FALSE;
    cam[cn].maxtimeout = DEFAULT_TIMOUT;
    cam[cn].image_status = EXP_ST_UNKNOWN;
    cam[cn].image_state = IDLE_STATE;
    cam[cn].image_total = 0;
    cam[cn].image_collected = 0;
    cam[cn].image_addr = NULL;
    cam[cn].intrStatus = 0L;
    cam[cn].lock = NO_LOCK;
//pairingvar=0;	/*test code*/

    if (cam[cn].state == STATE_OPEN)                                           /* Check if THIS camera is already open */
        return (-EBUSY);                                                       /* If so - then return busy - NO DICE!	*/
    /* With the Linux driver - each camera	*/
    /* is totally exclusive use!		*/
    filp->private_data = (void *)(&cam[cn]);                                   /* Set the Private data field in the Filp	*/
    /* struct to point to THIS cameras		*/
    /* structure for future use.			*/

    cam[cn].state = STATE_OPEN;                                                /* Set this Camera to BUSY			*/

    pvpci_ResetMBFlags( &cam[cn] );                                            /* Reset PCI Board interrupts..			*/

    pvpci_EnableIntrMB2( &cam[cn] );                                           /* Enable PCI Board mailbox2 interrupts..	*/

    /* due to legacy issues!)			*/

    return (DDI_SUCCESS);                                                      /* Indicate Successful Open			*/
}


/********************************************************************************
 * 										*
 * This FUNCTION is called when a process closes the				*
 * device file. It doesn't have a return value because 				*
 * it cannot fail. Regardless of what else happens, you 			*
 * should always be able to close a device (in 2.0, a 2.2			*
 * device file could be impossible to close).					*
 * 										*
 ********************************************************************************/
static int device_release(struct inode *inode, struct file *file)
{
    int cn;


    cn = inode->i_rdev & CAMERA_MINOR_MASK;                                    /* Mask off all but the camera number	*/
    cam[cn].xcmd_busy = FALSE;
    cam[cn].xcmd_timedout = FALSE;
    cam[cn].xfer_active = FALSE;
    cam[cn].maxtimeout = DEFAULT_TIMOUT;
    cam[cn].image_status = EXP_ST_UNKNOWN;
    cam[cn].image_state = IDLE_STATE;
    cam[cn].image_total = 0;
    cam[cn].image_collected = 0;
    cam[cn].image_addr = NULL;
    cam[cn].intrStatus = 0L;
    cam[cn].lock = NO_LOCK;
    cam[cn].state = STATE_CLOSED;                                              /* Mark this camera Closed		*/
    if (cam[cn].used_entries > 0)
    {                                                                          /* Check to see if the S/G Table is	*/
        /* still around for this camera		*/
        pvpci_free_buffer(&cam[cn]);                                           /* if it is - Go Free it!		*/
    }
    /* due to legacy issues!)			*/

    return (DDI_SUCCESS);                                                      /* Indicate Successful Close		*/
}


/********************************************************************************
 * 										*
 * This FUNCTION is called whenever a process which				*
 * has already opened the device file attempts to 				*
 * read from it.								*
 * 										*
 ********************************************************************************/
static ssize_t device_read(struct file *file,
                           char *buffer,                                       /* The buffer to fill with data	*/
                           size_t length,                                      /* The length of the buffer		*/
                           loff_t *offset)                                     /* Our offset in the file		*/
{
    return (DDI_SUCCESS);                                                      /* dummy function - not used */
}


/********************************************************************************
 * 										*
 * This function is called when somebody tries to				*
 * write into our device file.							*
 * 										*
 ********************************************************************************/
static ssize_t device_write(struct file *file,
                            const char *buffer,                                /* The buffer			*/
                            size_t length,                                     /* The length of the buffer	*/
                            loff_t *offset)                                    /* Our offset in the file	*/
{
    return (DDI_SUCCESS);                                                      /* dummy function - not used */
}


/********************************************************************************
 * 										*
 * This function is called whenever a process tries to				*
 * do an ioctl on our device file. We get two extra 				*
 * parameters (additional to the inode and file 				*
 * structures, which all device functions get): the number			*
 * of the ioctl called and the parameter given to the 				*
 * ioctl function.								*
 *										*
 * If the ioctl is write or read/write (meaning output 				*
 * is returned to the calling process), the ioctl call 				*
 * returns the output of this function.						*
 * 										*
 ********************************************************************************/
static int device_ioctl(struct inode *inode,
                        struct file *filp,
                        unsigned int ioctl_num,                                /* The number of the ioctl	*/
                        unsigned long ioctl_param)                             /* The parameter to it		*/
{
    cam_dev_ptr         camera;
    ioctl_wr_rd_type    wr_rd_data;
    ioctl_im_actv_type  im_actv;
    ioctl_im_stat_type  im_stat;
    int                 return_val;
    int         i;
    unsigned long       bytes_remaining, bsize, user_addr;
    uns16               data_in, data_out;
    uns32           pci_rev;


    return_val = DDI_SUCCESS;                                                  /* Start with a successful return code */

    /* Retrieve the Private Data (Cam Struct) for this camera*/
    camera = (cam_dev_ptr)(filp->private_data);


    /* SWITCH according to the ioctl called */

    switch (ioctl_num)
    {
        case IOCTL_WRITE_READ:                                                 /* Write data - Read data */

            if (copy_from_user(&wr_rd_data,(caddr_t)(ioctl_param),             /* Retrieve the data to write	*/
                               sizeof(ioctl_wr_rd_type)) != DDI_SUCCESS)
            {                                                                  /* From user space		*/

                printk(KERN_INFO  "ioctl: ddi_copyin failed for ioctl_wr_rd_type");
                camera->xcmd_busy = FALSE;
                camera->xcmd_timedout = FALSE;
                camera->xfer_active = FALSE;
                camera->maxtimeout = DEFAULT_TIMOUT;
                camera->image_status = EXP_ST_UNKNOWN;
                camera->image_state = IDLE_STATE;
                camera->image_total = 0;
                camera->image_collected = 0;
                camera->image_addr = NULL;
                camera->intrStatus = 0L;
                camera->lock = NO_LOCK;
                camera->state = STATE_CLOSED;                                  /* Mark this camera Closed		*/
                return(-EFAULT);                                               /* Return an Error to the caller	*/
            }

            /* Call the internal function that does the Write/Read call */

            return_val = pvpci_write_read( filp, (ioctl_wr_rd_ptr) &wr_rd_data );
            if (return_val != DDI_SUCCESS)
            {
                printk(KERN_INFO "ioctl: pvpci_write_read failed %04X\n", return_val);
                camera->xcmd_busy = FALSE;
                camera->xcmd_timedout = FALSE;
                camera->xfer_active = FALSE;
                camera->maxtimeout = DEFAULT_TIMOUT;
                camera->image_status = EXP_ST_UNKNOWN;
                camera->image_state = IDLE_STATE;
                camera->image_total = 0;
                camera->image_collected = 0;
                camera->image_addr = NULL;
                camera->intrStatus = 0L;
                camera->lock = NO_LOCK;
                camera->state = STATE_CLOSED;                                  /* Mark this camera Closed		*/
                return (-EIO);
            }
            break;

        case IOCTL_GET_MAXRETRIES:                                             /* Get the Camera Maximum Retry Count ALWAYS 1 for Linux*/
            data_out = 1;
            copy_to_user( (caddr_t)ioctl_param, (caddr_t)&data_out, sizeof(data_out));
            break;

        case IOCTL_SET_MAXRETRIES:                                             /* Set the Camera Maximum Retry - Ignores it	*/
            copy_from_user((caddr_t)&data_in, (caddr_t)ioctl_param, sizeof(data_in));
            break;

        case IOCTL_GET_MAXTIMEOUT:                                             /* Get the Maximum Timeout Value	*/
            data_out = camera->maxtimeout;
            copy_to_user( (caddr_t)ioctl_param, (caddr_t)&data_out, sizeof(data_out));
            break;

        case IOCTL_SET_MAXTIMEOUT:                                             /* Set the Maximum Timeout Value	*/
            copy_from_user((caddr_t)&data_in, (caddr_t)ioctl_param, sizeof(data_in));
            camera->maxtimeout = (data_in/2);                                  /*converting milliseconds to jiffies*/
            break;

        case IOCTL_SET_IMAGE_ACTIVE:                                           /* Set the Image Acquisition to Start	*/

            /* We are going to get the number of bytes of data to receive */

            if (copy_from_user((caddr_t)&im_actv, (caddr_t)ioctl_param,
                               sizeof(ioctl_im_actv_type)) != DDI_SUCCESS)
            {
                printk(KERN_INFO  "ioctl: ddi_copyin failed for image active\n" );
                camera->xcmd_busy = FALSE;
                camera->xcmd_timedout = FALSE;
                camera->xfer_active = FALSE;
                camera->maxtimeout = DEFAULT_TIMOUT;
                camera->image_status = EXP_ST_UNKNOWN;
                camera->image_state = IDLE_STATE;
                camera->image_total = 0;
                camera->image_collected = 0;
                camera->image_addr = NULL;
                camera->intrStatus = 0L;
                camera->lock = NO_LOCK;
                camera->state = STATE_CLOSED;                                  /* Mark this camera Closed		*/
                return(EFAULT);                                                /* Copy in Failed!			*/
            }
            camera->image_state = ACTIVE_STATE;                                /* Set Camera State to Active		*/
            camera->image_collected = 0;                                       /* Set amount collected to Zero		*/
            camera->image_status = EXP_ST_NO_DATA_A;                           /* Set Status to No Data Available	*/
            camera->image_addr = (uns8_ptr)im_actv.pix_array;                  /* Store off the User Buffer Address	*/
            return_val = DDI_SUCCESS;                                          /* Preset return code to Success	*/

            if (camera->used_entries == 0)
            {                                                                  /* If there are no S/G Entries then go allocate	*/  

                return_val = pvpci_create_buffer(camera, im_actv.totl_bytes);  /* Allocate	*/

                /* Otherwise the Buffer still exists so release it and recreate it */

                /* If already same size USE IT!	*/
            } else if (camera->image_total != im_actv.totl_bytes)
            {
                pvpci_free_buffer(camera);                                     /* Release it	*/
                return_val = pvpci_create_buffer(camera, im_actv.totl_bytes);  /* Recreate it	*/
            }
            camera->image_total = im_actv.totl_bytes;                          /* Store off the total bytes to collect	*/
            camera->xfer_active = TRUE;                                        /* Set the Transfer Active Flag to ACTIVE*/

            if (return_val != DDI_SUCCESS)
            {                                                                  /* Check for any errors */
                printk(KERN_INFO  "pvpci_ioctl: setup transfer failed\n" );

                camera->xcmd_busy = FALSE;
                camera->xcmd_timedout = FALSE;
                camera->xfer_active = FALSE;
                camera->maxtimeout = DEFAULT_TIMOUT;
                camera->image_status = EXP_ST_UNKNOWN;
                camera->image_state = IDLE_STATE;
                camera->image_total = 0;
                camera->image_collected = 0;
                camera->image_addr = NULL;
                camera->intrStatus = 0L;
                camera->lock = NO_LOCK;
                camera->state = STATE_CLOSED;                                  /* Mark this camera Closed		*/
                return(-EFAULT);
            }

            /* This is where the starting address of the Scatter Gather Table is		*/
            /* written to the PCI Controller - This gets things ready to start the chain of */
            /* events that causes the Pixel data to be moved into our KERNEL buffers. 	*/
            /* Once the transfer is complete - It MUST be moved from KERNEL buffers 	*/
            /* to the USER buffer.  -  This occurs in the Check Status IOCTL call later.	*/
            /* The transfer is started by a write of 0x2E to the PCI board and Camera	*/

            outl(camera->table_addr, (unsigned short)(camera->base_address[2]-1));
            break;

        case IOCTL_SET_IMAGE_IDLE:                                             /* Sets the Acqyisition Status to IDLE */

            camera->image_state     = IDLE_STATE;                              /* Set Image state to IDLE	*/
            camera->image_status    = EXP_ST_NO_DATA_I;                        /* Set Status to No Data	*/
            camera->image_total     = 0;                                       /* Set the total bytes to collect to Zero*/
            camera->image_collected = 0;                                       /* Set Amount Collected to Zero		*/ 
            camera->image_addr      = NULL;                                    /* Set the User Buffer Address to Zero	*/
            camera->xfer_active     = FALSE;                                   /* Set Transfer Active Flag to INACTIVE	*/
            break;

        case IOCTL_GET_IMAGE_STATUS:                                           /* Gets the Image Acquisition Status */

            /* This is the routine that checks for DMA complete and ends up copying		*/
            /* The KERNEL buffers to the USER buffer - This is done after the status	*/
            /* Check gets notified from the Interrupt routine that the transfer is		*/
            /* Complete - Or an Error occurred.						*/

            if (copy_from_user((caddr_t)&im_stat, (caddr_t)ioctl_param,        /* Get the User Struct */
                               sizeof(ioctl_im_stat_type)) != DDI_SUCCESS)
            {
                printk(KERN_INFO  "ioctl: ddi_copyin failed for image status\n");
                camera->xcmd_busy = FALSE;
                camera->xcmd_timedout = FALSE;
                camera->xfer_active = FALSE;
                camera->maxtimeout = DEFAULT_TIMOUT;
                camera->image_status = EXP_ST_UNKNOWN;
                camera->image_state = IDLE_STATE;
                camera->image_total = 0;
                camera->image_collected = 0;
                camera->image_addr = NULL;
                camera->intrStatus = 0L;
                camera->lock = NO_LOCK;
                camera->state = STATE_CLOSED;                                  /* Mark this camera Closed		*/
                return( -EFAULT );
            }
            im_stat.byte_cnt = camera->image_collected;                        /* Report how much is done	*/
            im_stat.status   = camera->image_status;                           /* Report the current Acq Status	*/
            if ((camera->xfer_active) &&                                       /* If We are currently in Acquisition mode	*/
                (camera->image_status == EXP_ST_COLLECT_DONE_I))
            {                                                                  /* And The ISR says DONE!	*/
                camera->xfer_active = FALSE;                                   /* Set Transfer Active Flag to IMACTIVE	*/
                im_stat.byte_cnt = camera->image_total;                        /* Update Amount Done with Total Acq	*/
                if (camera->used_entries > 0)
                {                                                              /* Make sure that there is a Scatter Table	*/
                    user_addr = (unsigned long)(camera->image_addr);           /* Get the User Address */

                    /* Init the Byte Remaining local variable */
                    bytes_remaining = camera->image_total;

                    /* Run a loop to traverse the S/G Table */
                    for (i=0; i<camera->used_entries; i++)
                    {

                        /* Test for a short block */
                        if (bytes_remaining > camera->pTE[i].b_size)
                        {

                            /* Not short so set to PAGE_SIZE */
                            bsize = camera->pTE[i].b_size;
                        } else bsize = bytes_remaining;                        /* Is Short Block so set to remaining */

                        if (copy_to_user((caddr_t)user_addr,                   /* Copy the S/G Page to User */

                                         /* Remember that we must use a Virtual Address to reach the data */
                                         /* To copy to the user buffer - The address of the page is stored*/
                                         /* as a BUS address in the S/G table because the DMA hardware 	 */
                                         /* Needs a BUS address when it is blasting data into the Page	 */

                                         (caddr_t)(bus_to_virt(camera->pTE[i].pAddr)),
                                         bsize) != DDI_SUCCESS)
                        {
                            printk(KERN_INFO  "ioctl: ddi_copyout failed for image status\n" );
                            camera->xcmd_busy = FALSE;
                            camera->xcmd_timedout = FALSE;
                            camera->xfer_active = FALSE;
                            camera->maxtimeout = DEFAULT_TIMOUT;
                            camera->image_status = EXP_ST_UNKNOWN;
                            camera->image_state = IDLE_STATE;
                            camera->image_total = 0;
                            camera->image_collected = 0;
                            camera->image_addr = NULL;
                            camera->intrStatus = 0L;
                            camera->lock = NO_LOCK;
                            camera->state = STATE_CLOSED;                      /* Mark this camera Closed		*/
                            return( -EFAULT );
                        }
                        user_addr = user_addr + bsize;                         /* Increment the User Buffer Pointer */
                        bytes_remaining = bytes_remaining - bsize;             /* Subtract the block we */
                        /* just moved from the total bytes remaining to be processed and then */
                        /* Lets go do it again! */

                    }                                                          /* For Loop Match for Data Mover */

                }                                                              /* If there is No data in the S/G Table */

            }                                                                  /* If the DMA is done */


            /*********************************************************
             * Must copy out for 2 fields are set in this structure. *
             *********************************************************/
            /* We are getting ready to copy the status data back to the user */

            if (copy_to_user((caddr_t)ioctl_param, (caddr_t)&im_stat,
                             sizeof(ioctl_im_stat_type)) != DDI_SUCCESS)
            {
                printk(KERN_INFO  "ioctl: ddi_copyout failed for image status\n" );
                camera->xcmd_busy = FALSE;
                camera->xcmd_timedout = FALSE;
                camera->xfer_active = FALSE;
                camera->maxtimeout = DEFAULT_TIMOUT;
                camera->image_status = EXP_ST_UNKNOWN;
                camera->image_state = IDLE_STATE;
                camera->image_total = 0;
                camera->image_collected = 0;
                camera->image_addr = NULL;
                camera->intrStatus = 0L;
                camera->lock = NO_LOCK;
                camera->state = STATE_CLOSED;                                  /* Mark this camera Closed		*/
                return( -EFAULT );
            }
            break;

        case IOCTL_GET_PIXTIME:                                                /* Gets the Pixel Time - Always returns 500	*/
            data_out = PIXTIME;                                                /* Load the Constant	*/
            copy_to_user((caddr_t)ioctl_param,                                 /* Copy it to the User	*/
                         (caddr_t)&data_out, sizeof(data_out));
            break;

        case IOCTL_GET_DRIVER_VER:                                             /* Get the Driver Version Number		 */
            data_out = DRIVER_VERSION;                                         /* Load the Driver Version Constant */
            copy_to_user((caddr_t)ioctl_param,                                 /* Copy it to the User		*/
                         (caddr_t)&data_out, sizeof(data_out));
            break;

        case IOCTL_GET_IMAGE_GRANUL:                                           /* Get the Image Granularity			*/
            data_out = MIN_XFER_SIZE;                                          /* Load a Constant	*/
            copy_to_user((caddr_t)ioctl_param,                                 /* Copy it to the User	*/
                         (caddr_t)&data_out, sizeof(data_out));
            break;

        case IOCTL_GET_INFO_LENGTH:                                            /* These are Unsupported at this time		*/
        case IOCTL_GET_INFO_STR:
            printk(KERN_INFO  "ioctl: string information functions not implemented" );
            break;


            /********************************************************
             * 							*
             * Left in here for possible reference in pvcam.	*
             * This was used in the previous driver.		*
             * 							*
             ********************************************************/

        case IOCTL_GET_VERBOSE_MODE:                                           /* These are Unsupported at this time		*/
        case IOCTL_SET_VERBOSE_MODE:
            printk(KERN_INFO  "ioctl: verbose mode functions not implemented" );
            break;

        case IOCTL_GET_PCI_FW_REV:                                             /* Get PCI firmware revision number		*/

            /* Get the Interrupt Status	*/
            pci_rev = inl((unsigned short)(camera->base_address[0] + MAILBOX4-1));
            copy_to_user((caddr_t)ioctl_param,                                 /* Copy it to the User */
                         (caddr_t)&pci_rev, sizeof(pci_rev));
            break;

        case IOCTL_LOAD_PCI_FLASH:
            
		if (copy_from_user(&wr_rd_data,(caddr_t)(ioctl_param),             /* Retrieve the data to write	*/
                               sizeof(ioctl_wr_rd_type)) != DDI_SUCCESS)

            	{                                                                  /* From user space		*/

                	printk(KERN_INFO  "ioctl: ddi_copyin failed for IOCTL_LOAD_PCI_FLASH");
                	return(-EFAULT);                                               /* Return an Error to the caller	*/
            	}

            return_val = loadPCIflash( filp, &wr_rd_data );
  	    printk(KERN_INFO  "IOCTL_LOAD_PCI_FLASH returned %d\n",return_val);
            break;


        case IOCTL_TEST:                                                       /*************test code left here for future testing*************/
//			printk("Command Complete Interrupt cnt %d\n",ccitrcnt);
//			printk("Transfer Complete Interrupt cnt = %d\n",tcitrcnt);
//			ccitrcnt = 0;
//			tcitrcnt = 0;
            break;




        default:                                                               /* If we get here - The user supplied an Invalid DDI IOCTL Code */
            printk(KERN_INFO  "ioctl: unrecognized command = %d.", ioctl_num );
            camera->xcmd_busy = FALSE;
            camera->xcmd_timedout = FALSE;
            camera->xfer_active = FALSE;
            camera->maxtimeout = DEFAULT_TIMOUT;
            camera->image_status = EXP_ST_UNKNOWN;
            camera->image_state = IDLE_STATE;
            camera->image_total = 0;
            camera->image_collected = 0;
            camera->image_addr = NULL;
            camera->intrStatus = 0L;
            camera->lock = NO_LOCK;
            camera->state = STATE_CLOSED;                                      /* Mark this camera Closed		*/
            return_val = EINVAL;
            break;
    }

    return (DDI_SUCCESS);                                                      /* Return a Successful Ioctl Call	*/
}

/*****************************************************************************
*
*  FUNCTION: PD_LOAD_PCIFLASH_Handler
*
*      Routine Description:
*              Handler for IO Control Code PD_LOAD_PCIFLASH
*
*      Parameters:
*              I - IRP containing IOCTL request
*
*      Return Value:
*              NTSTATUS - Status code indicating success or failure
*
*      Comments:
*              This routine implements the PD_LOAD_PCIFLASH function.
*              This routine runs at passive level.
*
*****************************************************************************/
int loadPCIflash (struct file *filp, ioctl_wr_rd_ptr ioctl_ptr)
{

    uns16       write_bytes;                                                   /* Allocate Local Variables for convenience	*/
    uns8_ptr    wKernel;
    uns16_ptr	wPCIfirmware;	
    cam_dev_ptr camera;
    int         status;
    unsigned long load_size, i;

    printk(KERN_INFO  "LOAD_PCIFLASH: Entering PCI downloader handler code\n");    
    
    /* Get camera pointer			*/
    camera = (cam_dev_ptr)(filp->private_data);                                /* Retrieve the Private Data		*/
    write_bytes = ioctl_ptr->write_bytes;                                       /* Number of bytes to Write			*/
/*    write_bytes = ByteCount;                                       Number of bytes to Write			*/
    wPCIfirmware = (uns16_ptr)ioctl_ptr->write_array;                                            /* Get the address of the user buffer (WRITE)	*/
    wKernel = camera->rwbuf;                                                   /* Get the address of the kernel buffer		*/

    status = DDI_SUCCESS;                                                       /* Default to successful operation */

    if (write_bytes > (1024*32))
    {                                                                          /* Check for the maximum write size allowed	*/
        printk(KERN_INFO  "PVPCI: write array[%d] OVERFLOW\n", write_bytes );
        status = -EINVAL;                                                      /* Write size is too Large	*/
    }

    /*********************************************************************
     * Begining of write operation.
     *********************************************************************/
                                                                               /* Is there data to be wrtten?			*/
    if ((write_bytes > 0) && (status==DDI_SUCCESS) )
    {                                                                          

                                                                                /* Must be if here */
	    printk(KERN_INFO "Writing Data to PCI cardi\n");
            load_size = write_bytes /2;
            outl(LOAD_PCIFLASH, (unsigned short)(camera->base_address[3]-1));      /* Send download command to the PCI card */
            for (i = 0; i < load_size; i++)                                      /* Now send the new firmware		*/
            {
                outl(*(wPCIfirmware+i), (unsigned short)(camera->base_address[3]-1));
            }         


    }


    /**********************************************************************
    *  End of Write operatation 
    ***********************************************************************/


    return(status);
}



/********************************************************************************
* 	FUNCTION to:								*
*										* 
* 	Initialize the module - Register the character device			*
*										*
*********************************************************************************/
static int __init pvpci_init_module(void)
{
    int ret_val, i;                                                            /* Local Scratch Vars				*/
    unsigned short cmdReg;                                                     /* Used to hold the Command Register Contents	*/
    struct pci_dev *boardp, *dev;                                              /* PCI Device Structure Pointers		*/
    uns8_ptr rwbp;                                                             /* This is the Write Read Buffer Pointer - Small Data */


    /* Register the character device (at least try) */
    printk(KERN_INFO "before register %d %s\n",MAJOR_NUM, DEVICE_NAME);
    ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);

    /* Negative values signify an error	*/
    if (ret_val < 0)
    {
        printk(KERN_INFO "%s failed with %d\n",
               "Sorry, registering the character device ", ret_val);
        return (ret_val);
    }
    dev = NULL;                                                                /* Set the PCI Device Pointer to NULL	*/
    boardp = NULL;                                                             /* Set the PCI Device 1 Pointer to NULL	*/
    camcnt = 0;                                                                /* Set the Internal Camera Counter to Zero */

    /* Go Find OUR PCI Card based on our Vendor ID and Device ID	*/

    dev = (struct pci_dev *)(pci_find_device(PMPCI_VENDOR_ID, PMPCI_DEVICE_ID, dev));

    while (dev != NULL)
    {                                                                          /* If Dev is not NULL - We found ONE	*/

        /* Clear the Camera Structure	*/
        memset(&cam[camcnt],0,sizeof(cam_dev_type));

        /* Read all four Base Address Registers of our Card	*/
        pci_read_config_word( dev, PCI_BASE_ADDRESS_0, &cam[camcnt].base_address[0] );
        pci_read_config_word( dev, PCI_BASE_ADDRESS_1, &cam[camcnt].base_address[1] );
        pci_read_config_word( dev, PCI_BASE_ADDRESS_2, &cam[camcnt].base_address[2] );
        pci_read_config_word( dev, PCI_BASE_ADDRESS_3, &cam[camcnt].base_address[3] );
        cam[camcnt].irq = dev->irq;                                            /* Store off the IRQ in the PCI Structure for easy access */

        /* Print out the 4 Base Addresses and IRQ at KERNINFO Level	*/
        printk(KERN_INFO "Base Address 0 0x%x\n",(unsigned int)cam[camcnt].base_address[0] );
        printk(KERN_INFO "Base Address 1 0x%x\n",(unsigned int)cam[camcnt].base_address[1] );
        printk(KERN_INFO "Base Address 2 0x%x\n",(unsigned int)cam[camcnt].base_address[2] );
        printk(KERN_INFO "Base Address 3 0x%x\n",(unsigned int)cam[camcnt].base_address[3] );
        printk(KERN_INFO "Using Irq 0x%x\n",(unsigned int)cam[camcnt].irq );            

        /* Make a copy of the PCI Device Structure and put it in our Camera Structure */
        boardp = (struct pci_dev *)(__get_free_pages(GFP_KERNEL,0));
        if (boardp != NULL)
        {
            memcpy(boardp,dev,sizeof(struct pci_dev));
            cam[camcnt].devp = boardp;
        } else return(-EFAULT);

        /* Allocate the Small Data Buffer used for Write Read */
        rwbp = (uns8_ptr)(__get_free_pages(GFP_KERNEL,RWBUF_NUM_PAGES));
        if (rwbp != NULL)
            cam[camcnt].rwbuf = rwbp;
        else
            return(-EFAULT);

        ret_val = request_irq(  dev->irq,                                      /* Hook the IRQ so Linux knows where to vector		*/
                                pvpci_intr,                                    /* Function to vector to				*/
                                SA_INTERRUPT | SA_SHIRQ,                       /* Share IRQ and Disable them when active	*/
                                DEVICE_NAME, 
                                &cam[camcnt]);

        pci_read_config_word(boardp, PCI_COMMAND, &cmdReg);                    /* Read the Config Register		*/          
        cmdReg |= PCI_COMMAND_MASTER;                                          /* Set the board to be a Bus Master	*/
        cmdReg |= PCI_COMMAND_IO;                                              /* Set up the board to use I/O not Mem Map */
        cmdReg &= ~PCI_COMMAND_MEMORY;                                         /* Clear the Mem map bit		*/
        pci_write_config_word(boardp, PCI_COMMAND, cmdReg);                    /* Write the Config Reg			*/
        pci_read_config_word(boardp, PCI_COMMAND,&cmdReg);                     /* Read it back				*/
        pci_set_master(boardp);                                                /* Make the system call to set MASTER	*/
        pvpci_DisableIntr( &cam[camcnt] );                                     /* Disable the card Interrupt Generator	*/
        pvpci_ResetMBFlags( &cam[camcnt] );                                    /* Clear the Mailbox Flags		*/
        pvpci_MailStatus( &cam[camcnt] );                                      /* Get the Mailbox Status		*/
        cam[camcnt].maxtimeout      = DEFAULT_TIMOUT;                          /* Set the default Timeout		*/
        cam[camcnt].xcmd_busy       = FALSE;                                   /* Set Command Busy to False		*/
        cam[camcnt].state           = STATE_CLOSED;                            /* Set Access to CLOSED			*/
        cam[camcnt].image_addr      = NULL;                                    /* Clear the Image Pointer		*/
        cam[camcnt].image_total     = 0;                                       /* Clear the Image Byte Count		*/
        cam[camcnt].image_collected = 0;                                       /* Set amount collected to Zero		*/
        cam[camcnt].image_state     = IDLE_STATE;                              /* Set Camera State to IDLE		*/
        cam[camcnt].image_status    = EXP_ST_UNKNOWN;                          /* Init the Status to UNKNOWN		*/
        cam[camcnt].xcmd_timedout = FALSE;
        cam[camcnt].xfer_active = FALSE;
        cam[camcnt].intrStatus = 0L;
        cam[camcnt].lock = NO_LOCK;



        /***************************************************************************/
        /* Lets allocate contiguous kernel pages for the Scatter Gather Table	   */
        /* Lets Review the Math for the Scatter Gather table and associated memory */
        /***************************************************************************
         *  Scatter Gather Table contains page entries for memory blocks.          *
         *  These memory blocks are PAGE_SIZE in size (4096 for PC Linux)          *
         *  Each entry in the Scatter Gather Table is 8 bytes long.                *
         *  4 Bytes for the Address of the Page - 4 Bytes for the size of the Page *
         *  									   *
         *  Therefore the size of the Scatter Gather table is directly             *
         *  Proportional to the total size of the stream of data that you are      *
         *  wanting to capture from the camera.                                    *
         *  									   *
         *  Total_Data_Transfer_Size = (((2 ^ TABLE_ORDER) * PAGE_SIZE) / 8) * 4096*
         *  The parameter passed to get_free_pages is Power of Two!		   *
         *  Therefore the CURRENT Total_Data_Transfer_Size is 32mb		   *
         *  									   *
         *  The Total_Data_Transfer_size is the MAXIMUM that the PCI card DMA can  *
         *  transfer in a single transaction.  The Scatter Gather table entries    *
         *  are populated based on the amount of data "expected" as told to us     *
         *  by the calling code (DDI by way of PVCAM)...			   *
         *  									   *
         * 									   * 
         ***************************************************************************/

        cam[camcnt].pTE = (TABLE_ENTRY *)(__get_free_pages(GFP_KERNEL,TABLE_ORDER));
        if (cam[camcnt].pTE == NULL) return(-EFAULT);                          /* Could not get the pages necessary	*/
        cam[camcnt].table_size      = TABLE_SIZE;                              /* Set to total number of S/G entries	*/

        /* We MUST get a Physical Address of the Scatter Gather table for the PCI DMA controller	*/
        /* As it needs this to be able to Bus Master the table.						*/

        cam[camcnt].table_addr      = (uns32)virt_to_phys(cam[camcnt].pTE);
        cam[camcnt].used_entries    = 0;                                       /* Initialize the number of used entries in the S/G */  
        init_waitqueue_head(&cam[camcnt].waitq);                               /* Initialize the Wait Queue mechanism used for Sleep */

        camcnt++;                                                              /* Bump the Camera Array Index for next Controller	*/

        /* Now lets search for the next controller (If there is one) and we will keep doing it		*/
        /* Until the Find Device says no more PCI cards							*/

        dev = (struct pci_dev *)(pci_find_device(PMPCI_VENDOR_ID, PMPCI_DEVICE_ID, dev));
    }                                                                          /* This is where we loop to do it again! */

    if (camcnt>0)
    {                                                                          /* If we found hardware - Print Information at KERN_INFO Level	*/

        printk(KERN_INFO "Pvpci driver loaded. The major device # is %d.\n",
               MAJOR_NUM);
        printk(KERN_INFO "We suggest you use:\n");
        for (i=0; i<camcnt; i++)
        {
            printk(KERN_INFO " mknod %s%d c %d %d\n", DEVICE_FILE_NAME, i, MAJOR_NUM, i);
        }
    }

    return (0);
}

/************************************************************************
 * FUNCTION								                                *
 * pvpci_write_read							                            *
 *									                                    *
 * The return value is passed up to ioctl, and directly out to the	    *
 * calling program.							                            *
 *									                                    *
 * Inputs: File Pointer							                        *
 *         Pointer to Ioctl Write/Read Structure			            *
 * Outputs: Data Out From Camera Along With Status			            *
 * Returns: DDI_SUCCESS							                        *
 *          EINVAL = no such device or address				            *
 *          EIO   = I/O error						                    *
 * Called By: IOCTL_WRITE_READ Case in Ioctl Function			        *
 * 		BUG HISTORY!!!!!!!!!!!!!!!!!!!!!			                    *
 * This function was returning the correct status values but the	    *
 * IOCTL function (caller) was not passing the status value back to	    *
 * the user - This was a bug that has been there since day ONE! (BAD)	*
 * Kudos to Anna Bahun (Consultant) for finding and fixing this!	    *
 ************************************************************************/
int pvpci_write_read(struct file *filp, ioctl_wr_rd_ptr ioctl_ptr)
{
    uns16       write_bytes;                                                   /* Allocate Local Variables for convenience	*/
    uns16       read_bytes;
    uns16       cmd_bytes;
    uns8_ptr    rUser, rKernel;
    uns8_ptr    wUser, wKernel;
    uns8        cmd_class;
    cam_dev_ptr camera;
    int             i;
    long        bc, lwords;

    union packdword
    {                                                                          /* This union is used to do		*/
        unsigned char bytes[4];                                                /* BIG/LITTLE Endian Conversions	*/
        unsigned long lword;
    } pd;
    /* Get camera pointer			*/
    camera = (cam_dev_ptr)(filp->private_data);                                /* Retrieve the Private Data		*/
    /* (Cam Struct) for this camera		*/
    /* Place ioctl variables into local variables for	*/
    /* convenience						*/
    write_bytes = ioctl_ptr->write_bytes;                                      /* Number of bytes to Write			*/
    cmd_bytes = write_bytes + 3;                                               /* Add in the overhead				*/
    wUser = ioctl_ptr->write_array;                                            /* Get the address of the user buffer (WRITE)	*/
    wKernel = camera->rwbuf;                                                   /* Get the address of the kernel buffer		*/
    read_bytes = ioctl_ptr->read_bytes;                                        /* Number of bytes to Read			*/
    cmd_class = ioctl_ptr->c_class;                                            /* Get the Class of the Command			*/
    rUser = ioctl_ptr->read_array;                                             /* Get the address of the user buffer (READ)	*/
    rKernel = camera->rwbuf;                                                   /* Get the address of the kernel buffer		*/
    /* Read and Write uses the same kernel buffer!	*/

    if (write_bytes > SCRIPTBUFFERSIZE-3)
    {                                                                          /* Check for the maximum write size	*/
                                                                               /* allowed				*/

        printk(KERN_INFO  "PVPCI: write array[%d] OVERFLOW", write_bytes );
        return( -EINVAL );                                                     /* Write size is too Large	*/
    }


    if (read_bytes && write_bytes)
    {                                                                          /* Check for Read and Write			*/

        /* Check if other write/read commands are running..	*/

        camera->lock = WRITE_LOCK;                                             /* Avoiding race condition	*/

        if (camera->xcmd_busy)
        {                                                                      /* Check to see if the Camera is already Busy	*/
            camera->xcmd_timedout = TRUE;                                      /* Set the Timeout Flag */              

            /* We are going to go to sleep at this point since the device is currently busy */
            /* We are going to sleep awaiting an interrupt from the PCI card		*/
            /* If we get the interrupt - The timedout flag will be set to FALSE		*/
            /* Allowing us to determine if we timed out or got the interrupt		*/
            /* Reference the pvpci_intr interrupt vector for the MAGIC!			*/

            interruptible_sleep_on_timeout( &camera->waitq, camera->maxtimeout);        


            if (camera->xcmd_timedout)
            {                                                                  /* Check if we "really" timed out	*/
                camera->xcmd_busy = FALSE;                                     /* Timed out - Set not busy and abort	*/
                printk(KERN_INFO  "PVPCI: Write/Read - Timed out waiting for previous Ack!\n" );
                return (-EIO);
            }
        }
        camera->xcmd_busy = TRUE ;                                             /* If the device is NOT busy - It is Now!		*/
    }

    /*********************************************************************
     * Setup the write.
     * Note, we have to prepend the command class
     * and the (unsigned short) array size before sending to the camera.
     * Hence, the offset to &wKernel[3]...
     *********************************************************************/

    if (write_bytes > 0)
    {                                                                          /* Do we have a Write to perform?			*/

//pairingvar++;

        if (copy_from_user(     /* Fetch the Data from the User to Write to the Camera	*/
                                (caddr_t)&(wKernel[3]),                        /* kernel space			*/
                                (caddr_t)wUser,                                /* user space			*/
                                (int)write_bytes ) != DDI_SUCCESS )
        {
            printk(KERN_INFO  "PVPCI: write array ERROR" );
            return( -EINVAL );                                                 /* Fetch Failed!		*/
        }
        wKernel[0] = cmd_class;                                                /* Put the Command Class at the head of the buffer */
        wKernel[1] = write_bytes >> 8;                                         /* Inject the Write Size			*/
        wKernel[2] = write_bytes & 0xff;                                       /* Do it the Motorola Way (ENDIAN)		*/

        if (camera->xfer_active)
        {                                                                      /* Have we committed to getting an Image?		*/

            /* If so - We are going to "peek"	*/
            /* at the request to see if we need	*/
            /* to ignore it or what			*/
            if ((wKernel[3] == 0x26) && (write_bytes == 5) &&
                (read_bytes == 10))
            {                                                                  /* call from pl_ccs_get_status()?	*/

                goto active_abort;                                             /* Yep - ignore it		*/

            } else if ((wKernel[3] == 0x2e) && (write_bytes == 1) &&
                       (read_bytes == 0))
            {                                                                  /* Detect Start of Exposure	*/
                /* call from pl_exp_start...()	*/
                /* OK.. let this pass		*/

            } else                                                             /* We are in the middle of an acquisition so DO	*/
                /* NOT ALLOW A WRITE				*/
                goto active_abort;
        }
        pd.lword = (unsigned long)(write_bytes);                               /* Load the Write Size into the converter*/
        wKernel[1] = pd.bytes[1];                                              /* reverse bytes to ...			*/
        wKernel[2] = pd.bytes[0];                                              /* motorola format			*/
        i=0;

        do
        {                                                                      /* Make sure that we pack the correct number of bytes	*/

            if (i < cmd_bytes) 
                pd.bytes[3]=wKernel[i];                         /* Pack in the data	*/
            else 
                pd.bytes[3] = 0;                                              /* If no data then send 0x00	*/
            i++;
            if (i < cmd_bytes) 
                pd.bytes[2]=wKernel[i];
            else 
                pd.bytes[2] = 0;
            i++;
            if (i < cmd_bytes) 
                pd.bytes[1]=wKernel[i];
            else 
                pd.bytes[1] = 0;
            i++;
            if (i < cmd_bytes) 
                pd.bytes[0]=wKernel[i];                         /* pack bytes into dword*/
            else 
                pd.bytes[0] = 0;
            i++;

            /* Now that the data is packed in, write	*/
            /* it to the PCI Card through the Mailbox	*/
            outl(pd.lword, (unsigned short)(camera->base_address[1]-1));

        } while ( i < cmd_bytes );                                             /* Keep going until all data sent		*/
    }
    camera->xcmd_timedout = TRUE;                                              /* Yes we are - default the timeout flag	*/
    interruptible_sleep_on_timeout( &camera->waitq, camera->maxtimeout);            

    if (camera->xcmd_timedout)
    {                                                                          /* Did we time out?				*/
        camera->xcmd_busy = FALSE;                                             /* Yep - Error					*/
        printk(KERN_INFO  "PVPCI: Write/Read - Timed out waiting for Write Ack!\n");
        return (-EIO);
    }

    /*********** End of Write Data Section***********/


    if (read_bytes > 0)
    {                                                                          /* Check to see if we want to Read Data			*/
        //pairingvar++;	/*test code*/
        bc = 0;
        lwords = read_bytes / 4;                                               /* Calculate the number of DWORDS to Read	*/
        if ((read_bytes % 4) != 0) lwords++;                                   /* If Not on Boundary - Add one		*/

        if (READ_LOCK == camera->lock)                                         /* See if the INT already happened - If not sleep */
        {
            camera->xcmd_timedout = TRUE;                                      /* Yes we are - default the timeout flag */
            interruptible_sleep_on_timeout( &camera->waitq, camera->maxtimeout);            
        }
        if (camera->xcmd_timedout)
        {                                                                      /* Did we time out?				*/
            camera->xcmd_busy = FALSE;                                         /* Yep - Error				*/
            printk(KERN_INFO  "PVPCI: Write/Read - Timed out waiting for Read Ack!\n");
            return (-EIO);
        }
        /* No timeout - Must have gotten an Interrupt			*/

        if (camera->intrStatus & 0xfff)
        {                                                                      /* Check our ISR Status word for an Error */
            printk(KERN_INFO  "PVPCI: communication Error\n");
            return (-EIO);                                                     /* Got an Error	*/
        }

        /********************************************************************************/
        /* CAVEAT! - We are Not Checking for how much data we are reading - This could	*/
        /* Cause an OVERFLOW if we read too much data and exceed the size of the kernel */
        /* Buffer! - Probably should protect this at some point!			*/
        /* FIXED as of 2/23/2003 see code following.					*/
        /********************************************************************************/
        if (read_bytes > RWBUF_NUM_BYTES)
        {
            printk(KERN_INFO "PVPCI: number of bytes requested to read overflow\n");
            return (-EIO);
        }

        for (i=0; i<lwords; i++)
        {                                                                      /* Read all the data requested			*/

            /* Get 32 Bits from Mailbox		*/
            pd.lword = inl((unsigned short)(camera->base_address[1]-1));

            rKernel[bc] = pd.bytes[3];                                         /* Transfer it to the kernel buffer	*/
            bc++;
            if (bc>=read_bytes) break;
            rKernel[bc] = pd.bytes[2];
            bc++;
            if (bc>=read_bytes) break;
            rKernel[bc] = pd.bytes[1];
            bc++;
            if (bc>=read_bytes) break;
            rKernel[bc] = pd.bytes[0];
            bc++;
            if (bc>=read_bytes) break;
        }                                                                      /* Keep going until we are done	*/

        if (copy_to_user(           /* Have all the data now, transfer it	*/
                                    /* to the user				*/
                                    (caddr_t) rUser,                           /* user space	*/
                                    (caddr_t) rKernel,                         /* kernel space	*/
                                    (int)read_bytes ) != DDI_SUCCESS )
        {
            printk(KERN_INFO  "PVPCI: read array ERROR");
            return( -EINVAL);
        }
    }
    return(DDI_SUCCESS);


    /********************************************************/
    /* LABEL to GOTO					*/
    /* Vectored from a point where we must get out quick	*/
    /********************************************************/
    active_abort:
    /* Log the Abort at KERN_INFO Levelq	*/
//		printk(KERN_INFO "ca=%d\n",camera->xfer_active);
    printk(KERN_INFO "pvpci_write_read - command 0x%x aborted %d %d\n",
           (int)wKernel[3], (int)write_bytes, (int)read_bytes );

    if (read_bytes > 0)
    {                                                                          /* If we need to abort - ZERO out 	*/
        /* the kernel and user buffer for the	*/
        /* read					*/

        for (i=0; i<read_bytes; i++) rKernel[i] = 0;
        if (copy_to_user(
                        (caddr_t) rUser,                                       /* user space	*/
                        (caddr_t) rKernel,                                     /* kernel space	*/
                        (int)read_bytes) != DDI_SUCCESS )
        {
            printk(KERN_INFO  "PMPCI: read array ERROR" );
            return( EINVAL);
        }
    }
    return(DDI_SUCCESS);
}



/************************************************************************/
/*									*/
/* FUNCTION to:								*/
/* Allocate image buffer in kernel space and build Scatter Gather Table	*/
/*									*/
/************************************************************************/
int pvpci_create_buffer(cam_dev_ptr cam, unsigned long buf_size)
{
    int nblocks,i;
    unsigned long bytes_remaining, bsize;


    if ((cam == NULL) || (buf_size == 0)) return (-EINVAL);                    /* Check call for Sanity		*/
    bsize = PAGE_SIZE * IMAGE_PAGES;                                           /* allocate memory in Contiguous Blocks	*/
    nblocks = (buf_size / bsize);                                              /* Calculate how many blocks for this image	*/
    /* (From User)					*/

    if ((nblocks * bsize) < buf_size) nblocks++;                               /* See if there is any left over		*/
    if (nblocks >= TABLE_SIZE) return (-ENOMEM);                               /* Make sure that our Scatter Gather Table	*/
    /* has enough Room 			   	*/
    memset(cam->pTE, 0, TABLE_SIZE*8);                                         /* Zero out the Scatter Gather Table 		*/
    bytes_remaining = buf_size;                                                /* Get ready to chunk through the blocks and	*/
    /* allocate the pages 				*/

    for (i=0; i<nblocks; i++)
    {                                                                          /* Run through the blocks				*/

        /* Allocate a "Block" for the S/G table - This	*/
        /* block is actually several pages but contiguous*/

        cam->pTE[i].pAddr = virt_to_bus((void *)(__get_free_pages(GFP_KERNEL, IMAGE_ORDER)));

        if (cam->pTE[i].pAddr != 0)
        {                                                                      /* Check if it worked 				*/

            if (bytes_remaining < bsize) bsize = bytes_remaining;              /* See if we have leftovers */
            cam->pTE[i].b_size = bsize;                                        /* Put the block size in the S/G Table	*/
            bytes_remaining = bytes_remaining - bsize;                         /* Subtract this one - and do again */

        } else
        {                                                                      /* We could not get the buffers allocated		*/

            cam->used_entries = i;                                             /* Save the number allocated so we can free what*/
            /* we got					*/

            printk(KERN_INFO "Image allocation failed\n");
            return (-ENOMEM);                                                  /* Report the Error	*/
        }

    }                                                                          /* Keep going until all processed and created			*/

    cam->used_entries = nblocks;                                               /* Put the number of used entries in the S/G Table in the Cam Struct */

    return (DDI_SUCCESS);
}


/****************************************************************/
/*								*/
/* FUNCTION to:							*/
/*								*/
/* Free The Kernel Space Image Buffer				*/
/*								*/
/****************************************************************/
void pvpci_free_buffer(cam_dev_ptr cam)
{
    int i;

    if (cam == NULL) return;                                                   /* Check for Sanity			*/

    /* There is No Buffer To Free		*/
    if ((cam->used_entries == 0) || (cam->pTE == NULL)) return;

    /* Run through the S/G Table and free	*/
    /* all blocks				*/
    for (i=0; i< cam->used_entries; i++)
    {

        if (cam->pTE[i].pAddr != 0) free_pages(
                                              (unsigned long)bus_to_virt(cam->pTE[i].pAddr), IMAGE_ORDER);

        /* After freeing the memory		*/
        cam->pTE[i].pAddr = 0;                                                 /* Set S/G Address to Zero		*/
        cam->pTE[i].b_size = 0;                                                /* Also set the size to zero		*/

    }                                                                          /* Keep going until finished */

    cam->used_entries = 0;                                                     /* Update the Cam Struct with no	*/

}


/****************************************************************
 * 								*
 * FUNCTION to:							*
 * 								*
 * Disable The PCI Mailbox 1 Interrupt				*
 * 								*
 ****************************************************************/
void pvpci_DisableIntr( cam_dev_ptr camera )
{

    if (!camera) return;                                                       /* Sanity Check		*/

    /* Write to the PCI Card */
    outl(INT_DISABLE, (unsigned short)(camera->base_address[0] + INTCSR-1));

}


/****************************************************************
 *								*
 * FUNCTION to:							*
 *								*
 * Enable The PCI Mailbox 1 Interrupt				*
 * 								*
 ****************************************************************/
void pvpci_EnableIntrMB1( cam_dev_ptr camera )
{

    if (!camera) return;                                                       /* Sanity Check		*/

    /* Write to the PCI Card */
    outl(INT_MAILBOX1, (unsigned short)(camera->base_address[0] + INTCSR-1));

}


/****************************************************************
 * 								*
 * FUNCTION to:							*
 * 								*
 * Enable The PCI Mailbox 2 Interrupt.				*
 * 								*
 ****************************************************************/
void pvpci_EnableIntrMB2( cam_dev_ptr camera )
{

    if (!camera) return;                                                       /* Sanity Check		*/

    /* Write to the PCI Card */
    outl(INT_MAILBOX2, (unsigned short)(camera->base_address[0] + INTCSR -1));

}

/****************************************************************
 *								*
 * FUNCTION to:							*
 *								*
 * Reset The PCI Mailbox Flags					*
 * 								*
 ****************************************************************/
void pvpci_ResetMBFlags( cam_dev_ptr camera )
{

    if (!camera) return;                                                       /* Sanity Check		*/

    /* Write to the PCI Card */
    outl(MASTERRESET, (unsigned short)(camera->base_address[0] + MASTERCSR-1));

    /* Write to the PCI Card */
    outl(MASTERSET, (unsigned short)(camera->base_address[0] + MASTERCSR-1));

}

/****************************************************************
 * 								*
 * FUNCTION to:							*
 *								*
 * Get The Status Of PCI Mailboxes				*
 * 								*
 ****************************************************************/
long pvpci_MailStatus( cam_dev_ptr camera )
{
    u_long status;


    if (!camera) return (-EFAULT);                                             /* Sanity Check		*/

    /* Read from the PCI Card */
    status = inl((unsigned short)(camera->base_address[0] + MAILSTATUS-1));


    return (status);
}


/****************************************************************
 * 								*
 * FUNCTION to:							*
 * 								*
 * Abort The Exposure						*
 * 								*
 ****************************************************************/
void pvpci_AbortExp( cam_dev_ptr camera )
{


    if (!camera) return;                                                       /* Sanity Check		*/

    /* Write to the PCI Card */
    outl(ABORT_EXP, (unsigned short)(camera->base_address[0] + OUTMAILBOX2-1));

}


/****************************************************************
 * 								*
 * FUNCTION 							*
 * 								*
 * This Function Handles The Mailbox Interrupts.		*
 * This is the Vector that we set the PCI Card to Generate	*
 * NOTE, since this is a HIGH LEVEL Interrupt,			*
 * we want to minimize processing in this context!		*
 * 								*
 ****************************************************************/
static irqreturn_t pvpci_intr ( int irq, void *dev_id, struct pt_regs *regs )
{                                                                              /* This is Where we go when the PCI card wants us!	*/
    cam_dev_ptr camera;
    long has_mail, status, mailstatus, mb2status;
    /* Get the Camera Structure Associated with this interrupt	*/
    camera = (cam_dev_ptr)(dev_id);

    if (!camera)                                                               /* Big Time Sanity Check!				*/
        return IRQ_NONE;



    if (camera->irq != irq)                                                    /* Match up the Irq that just happened with the one in the	*/
        /* Camera Structure. They better be the same or it is not our	*/
        /* Interrupt							*/

        return IRQ_NONE;                                                                /* Not our interrupt					*/

    /* Read from the PCI Card				*/
    status = inl((unsigned short)(camera->base_address[0] + INTCSR-1));

    if (!(status & OUR_PCI_INTERRUPT))                                         /* Mask and Check for "OUR" Interrupt			*/
        return IRQ_NONE;                                                                /* Not our interrupt					*/

    /* Acknowledge The Interrupt To The PCI Board......	*/

    /* Write to the PCI Card				*/
    outl(INT_MAILBOX2, (unsigned short)(camera->base_address[0] + INTCSR-1));

    /* Check for Any Mail in the PCI Mailbox....		*/
    /* Get the Mail Status					*/
    mailstatus = inl((unsigned short)(camera->base_address[0] + MAILSTATUS-1));

    /* Get the Interrupt Status				*/
    mb2status = inl((unsigned short)(camera->base_address[0] + MAILBOX2-1));

    has_mail = mailstatus & MAIL_FOR_US;                                       /* Mask and Check whether there is mail for us		*/

    if (has_mail)
    {                                                                          /* Yep - You've Got Mail (To quote a very large company!) ;=)	*/
        camera->intrStatus = mb2status;                                        /* Save the Mailbox Status in our Cam Structure		*/
        /* Interrupt generated by cmd transfer..		*/

        /* Mask and Check for Command Completion */
        if (camera->intrStatus & 0x8000)
        {
//ccitrcnt++;		/*test code counting Command Complete Interrupts*/
            /* Signal the Sleeper that we got the Interrupt... It is now Ok to Proceed	*/

            /* Code added to prevent a race condition	*/
//pairingvar--;	/*test code*/

            if (READ_LOCK == camera->lock)
            {
                camera->lock = NO_LOCK;
            } else if (WRITE_LOCK == camera->lock)
            {
                camera->lock = READ_LOCK;
            }
            camera->xcmd_busy = FALSE;                                         /* Clear the Command Busy Flag	*/
            camera->xcmd_timedout = FALSE;                                     /* Clear The Timeout Flag	*/  
            wake_up_interruptible(&camera->waitq );                            /* Wake up the Sleeper!		*/      
        } else
        {                                                                      /* It is not a Command Completion Interrupt but rather	*/
            /* something else					*/

            /* Image Transfer Completion???			*/
            /* Has the Image Transfer Completed? (DMA Done	*/
            if (camera->intrStatus == 0x00)
            {
//tcitrcnt++;	/*test code counting interrupts at DMA*/
                /* Yes - Set the Camera Status to Done		*/
                camera->image_status = EXP_ST_COLLECT_DONE_I;
            } else                                                             /* Well - It must be an error then....		*/
                /* Check For Errors and Report them in the	*/
                /* Camera Status Structure..			*/
                if (camera->intrStatus & 0x02)
                camera->image_status = EXP_ST_MISSING_DATA_I;
            else if (camera->intrStatus & 0x10)
                camera->image_status = EXP_ST_EXTRA_DATA_I;
            else if (camera->intrStatus & 0x40)
                camera->image_status = EXP_ST_EXTRA_DATA_I;
            else if (camera->intrStatus & 0x80)
                camera->image_status = EXP_ST_FIFO_OVER_A;
            else
                camera->image_status = EXP_ST_XFER_ERR_I;
        }
    }

    return IRQ_HANDLED;
}

/************************************************************************/
/*									*/
/*	FUNCTION to:							*/
/* Cleanup - unregister the appropriate file from /proc			*/
/* This is called when the kernel module is unloaded			*/
/* (typically by using RMMOD 						*/
/*									*/
/************************************************************************/
static void __exit pvpci_cleanup_module(void)
{
    int ret,i;


    if (camcnt>0)
    {                                                                          /* Make sure we have some hardware			*/
        for (i = 0; i < camcnt; i++)
        {                                                                      /* If we do have hardware - Do this for each one */
            if (cam[i].pTE != NULL)
            {                                                                  /* If we have a valid address for the S/G Table	*/
                pvpci_free_buffer(&cam[i]);                                    /* Go free all of the pages referenced in the S/G Table */
                free_pages((unsigned long)(cam[i].pTE), TABLE_ORDER);          /* Free the actual S/G Table */
            }
            free_irq(cam[i].irq,&cam[i]);                                      /* Release the Vector for the Interrupt		*/
            /* Free the Write/Read Buffer	*/
            if (cam[i].rwbuf != NULL) free_pages((unsigned long)(cam[i].rwbuf),1);
            /* Free the Device Block Structures*/
            if (cam[i].devp != NULL) free_pages((unsigned long)(cam[i].devp),0);
        }
    }
    /* Unregister the device		*/
    ret = unregister_chrdev(MAJOR_NUM, DEVICE_NAME);

    /* If there's an error, report it	*/
    if (ret < 0)
        printk(KERN_INFO  "Error in module_unregister_chrdev: %d\n", ret);
    else
        printk(KERN_INFO  "%s removed\n", DEVICE_FILE_NAME);

}  


module_init(pvpci_init_module);
module_exit(pvpci_cleanup_module);
