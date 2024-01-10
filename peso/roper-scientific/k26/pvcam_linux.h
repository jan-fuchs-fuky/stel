/******************************************************************************
 *
 * pvcam_linux.h
 *
 * LINUX SPECIFIC HEADER FILE
 *
 * Copyright (C) Roper Scientific  1992-2002.  All rights reserved.
 *
 ******************************************************************************
 *
 * $Archive: /PVCAM/SourceLinux/pvcam_linux.h $
 * $Revision: 1 $
 * $Date: 2/07/02 9:17a $
 * $Author: Rgroom $
 * $History: pvcam_linux.h $
 * 
 * *****************  Version 1  *****************
 * User: Rgroom       Date: 2/07/02    Time: 9:17a
 * Created in $/PVCAM/SourceLinux
 *
 *****************************************************************************
 *
 * History:
 *
 * IN - YYYY/MM/DD - Change
 * ---------------------------------------------------------------------------
 * RG - 2002/01/23 - Created from old structure
 *
 *****************************************************************************/

#ifndef _PVCAM_LINUX_H
#define _PVCAM_LINUX_H

static const char *_pvcam_linux_h_="$Header: /PVCAM/SourceLinux/pvcam_linux.h 1     2/07/02 9:17a Rgroom $";

#define MAJOR_NUM         61


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



typedef struct                 /* Used to pass information in and out of ioctl */
{
  uns8     c_class;          /* command class for this write-read instruction*/
  uns16    write_bytes;      /* number of bytes to write: 1-32768            */
  uns8_ptr write_array;      /* location of data to send                     */
  uns16     read_bytes;      /* number of bytes to read:  0-32768 (0=no read)*/
  uns8_ptr  read_array;      /* location to place date read                  */
} ioctl_wr_rd_type, *ioctl_wr_rd_ptr;


typedef struct               /* Used to pass information in and out of ioctl */
{
  uns32     totl_bytes;      /* number bytes to collect (up to several meg)  */
  uns16_ptr pix_array;       /* location to put data into                    */
} ioctl_im_actv_type, *ioctl_im_actv_ptr;


typedef struct                /* Used to pass information in and out of ioctl */
{
  uns32  byte_cnt;            /* number bytes collected so far                */
  int    status;              /* follows codes frm v00incl_.h (and scsi_cam.h)*/
} ioctl_im_stat_type, *ioctl_im_stat_ptr;

/******************************************************************************
 * ioctl_info_str_type used to pass the information string into and
 * out of ioctl.
 *   number of bytes
 *   character string
 *   error code
 ******************************************************************************/
typedef struct
{
  uns16     bytes;
  char     *info_str;
  int16     error_value;
}  ioctl_info_str_type, *ioctl_info_str_ptr;


/****************************************************************************
* DEBUGGING FUNCTIONS - following common C practice, "NDEBUG" is defined for
* production code, to litterally say "no debugging".  If NDEBUG hasn't been
* defined, we insert special error checking code.
*      In this case, the code that's inserted allocated extra space every
* time someone asks for system memory.  It allocates a few bytes at the
* front, and a few more at the back of every memory segment.  These are then
* filled with a known pattern.  This operation is performed in
* "pv_memory_setup".  The actual malloc routines execute the macro
* "SETUP_MEM".  As can be seen below, this completely drops out when NDEBUG
* is defined.  During the free operation, the macro "DEBUG_MEM" is called.
* This then calls the function "pv_memory_check", which will examine the
* patterns at the front and back end of the allocated memory, and make sure
* that they haven't been altered.
*      It is assumed that the vast majority of memory errors will occur from
* writing past the end of an array or a memory space.  This will catch
* attempts to write past the end of this memory chunk, as well as overwrites
* by variables or arrays just prior to this section.
****************************************************************************/


#if defined NDEBUG /*** Dummy versions drop out in production code ***********/

#ifndef MEM_TEST_OFS
#define MEM_TEST_OFS    0         /* amount of pattern bytes BEFORE memory   */
#endif
#ifndef MEM_TEST_SIZE
#define MEM_TEST_SIZE   0         /* total amount of extra mem with patterns */
#endif
#define DEBUG_MEM(block,heap,msg)
#define SETUP_MEM(pointer,size)

#else    /* NDEBUG **** Extra code added for debugging ***********************/
#ifndef MEM_TEST_OFS
#define MEM_TEST_OFS   16         /* amount of pattern bytes BEFORE memory   */
#endif
#ifndef MEM_TEST_SIZE
#define MEM_TEST_SIZE  32         /* total amount of extra mem with patterns */
#endif

void_ptr pv_memory_setup( void_ptr mem_start, uns32 size );
boolean pv_memory_check( void_ptr block, int16 heap );

#define SETUP_MEM(pointer,size)  (pointer)=pv_memory_setup((pointer),(size))

#define DEBUG_MEM(block,heap,msg) { \
    DEBUG("%s:",(msg));               \
        pv_memory_check((block),(heap));  \
}

#endif   /* NDEBUG ***********************************************************/


#endif /* _PVCAM_LINUX_H */
