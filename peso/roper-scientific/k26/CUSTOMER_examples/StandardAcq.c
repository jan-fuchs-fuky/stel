#include <stdio.h>
#include <stdlib.h>
#include "master.h"
#include "pvcam.h"

#include <string.h>
#include <unistd.h>        /* required for "access"                          */
#include <dirent.h>        /* directory information                          */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <tiff.h>
#include <tiffio.h>

//#include <RS.h>            /* system-dependant private needs, other includes  */
                           /* Hidden function prototypes, constants, & macros */
                           /* The files above must be retained in this EXACT  */
                           /* ORDER -- some systems precompile those headers  */

//#include <RS00.h>
//#include <RS03.h>
#include "pvcam_linux.h"
//#include "/usr/local/Roper/driver/current/pvpci.h"


#define CAPTURE_SCRIPTS
#define EXP_ST_IDLE	1
#define EXP_ST_COLLECT_DONE_I	103




void QuickTest(void);	// Temporary code to test Receive Data

static void AcquireStandard( int16 hCam );

int main(int argc, char **argv)
{
    int16 hCam;                     /* camera handle                  */
    char cam_name[CAM_NAME_LEN];    /* camera name                    */
//    int16 hCam;                     /* camera handle                  */

//    while (1) {
    /* Initialize the PVCam Library and Open the First Camera */
    pl_pvcam_init();
    pl_cam_get_name( 1, cam_name );
    pl_cam_open(cam_name, &hCam, OPEN_EXCLUSIVE );

    AcquireStandard( hCam );

    pl_cam_close( hCam );

    pl_pvcam_uninit();



 //   pl_pvcam_init();
 //   pl_cam_get_name( 1, cam_name );
 //   pl_cam_open(cam_name, &hCam, OPEN_EXCLUSIVE );

 //   AcquireStandard( hCam );

 //   pl_cam_close( hCam );

 //   pl_pvcam_uninit();

 //   }

    return 0;
}

void AcquireStandard( int16 hCam )
{
    rgn_type region = { 0, 511, 1, 0, 511, 1 };
    uns32 size;
    uns16 *frame;
    int16 status;
    uns32 not_needed;
//    uns16 numberframes = 10;
    uns16 numberframes = 10000;
    
    /* Init a sequence set the region, exposure mode and exposure time */
    pl_exp_init_seq();
    pl_exp_setup_seq( hCam, 1, 1, &region, TIMED_MODE, 10000, &size );

    frame = (uns16*)malloc( size );

    /* Start the acquisition */
//    printf( "Collecting %i Frames\n", numberframes );
    
    /* ACQUISITION LOOP */
//	while( numberframes ) {
        pl_exp_start_seq(hCam, frame );

        /* wait for data or error */
//        while( pl_exp_check_status( hCam, &status, &not_needed ) && 
//      	(status != READOUT_COMPLETE && status != READOUT_FAILED) );
QuickTest();

        /* Check Error Codes */
//        if( status == READOUT_FAILED ) {
//            printf( "Data collection error: %i\n", pl_error_code() );
 //           break;
//        }

        /* frame now contains valid data */
 //       printf( "Center Three Points: %i, %i, %i\n", 
//                frame[size/sizeof(uns16)/2 - 1],
 //               frame[size/sizeof(uns16)/2],
  //              frame[size/sizeof(uns16)/2 + 1] );
  //      numberframes--;
//        printf( "Remaining Frames %i\n", numberframes );
    //} /* End while */

    /* Finish the sequence */
    pl_exp_finish_seq( hCam, frame, 0);
    
    /*Uninit the sequence */
    pl_exp_uninit_seq();

    free( frame );
}


void 
QuickTest(void)	// Temporary code to test Receive Data
{

	unsigned char buff[1024] = {0};
	unsigned char *array = &buff[3];
	unsigned char outbuff[1024];
	unsigned long *ptr;
  	ioctl_wr_rd_type ioctl_val;
	int retcode;
	
	enum
	{ SPD_BIT_DEPTH,          SPD_MAX_GAIN,               SPD_SPEED,
	  CAM_SER_NUM,             COOLING,                    CAM_NAME,
	  CLEAR_CYCLES,            CUR_CCD_TEMP,               FRAME_CAP,
	  FULL_WELL_CAP,           MPP_CAP,  			PAR_SIZE,
	  PAR_PIX_SIZE,            PAR_PIX_SPACE,      		SER_PIX_SIZE,			
	 SER_PIX_SPACE,           POST_MASK,                POSTSCAN,
	  PRE_MASK,                PREAMP_DELAY,               PRE_FLASH,
	  PRESCAN,                 SER_SIZE,                   SUM_WELL_PRESENT,
	  TEMP_RANGE,              TOTAL_SPD,                  TOTAL_PORTS,
	  SPD_USER_GAIN,           ERROR_LOG,                  STAT_TABLE,
	  STAT_TABLE_SIZE,         SH_OPEN_DELAY,              SH_CLOSE_DELAY,
	  SUBSYS_ID,               SUBSYS_NAME,                CCD_TEMP_SET,
	  FIRMWARE_VER,            READ_PORT_NUM,              CLEAR_STAT_BUF,
	  STAT_BUF_SIZE,           SEND_STAT_BUF,              CUR_SPD,
	  SET_TIME_PER_PIX,        SELECT_SPD,                 SET_SPD_GAIN,
	  SET_TEMP,                EXECUTE_CCL,                SUBSYS_DIAG,
	  SET_SH_OPEN_DELAY,       SET_SH_CLOSE_DELAY,         CLEAR_ERROR_LOG,
	  STOP_CCS,                DEBUG_CMD,                  GET_SER_REG_FULL_WELL,
	  GET_SUM_WELL_FULL_WELL,  GET_CCL_SCRIPT_WORDS,     FIRMWARE_MINOR_VER,
	  SET_EXP_TIME,            EXP_TIME,                 SEND_CAPABILITIES,
	  GET_OFFSET,              SET_OFFSET,                AVL_RESOLTNS,
	  MIN_RESOLTN,             SEND_RESOLUTIONS,           SEND_AVA_PORTS,
	  SEND_PORTS_RESL,         SEND_PORTS_MIN,             SEND_PORTS_MAX,
	  SEND_PORTS_DIR,          SEND_PORTS_TYPE,            GET_CURRENT_PORT_ST,
	  SET_CURRENT_PORT_ST,     SEND_STAT_BUFFER_MAX_SIZE,  SET_DIRECTION,
	  SEND_PORTS_ATTRIBUTES,   ALPHA_SER_NUM,              SET_PAR_SIZE,
	  SET_SER_SIZE,            SET_PRE_MASK,               SET_POST_MASK,
	  SET_PRESCAN,             SET_POSTSCAN,               GET_MAX_GAIN_MULT_FACTOR,
	  GET_GAIN_MULT_FACTOR,    SET_GAIN_MULT_FACTOR
	};

	buff[0] = '?';
	buff[1] = 0;
	buff[2] = 10;
		
	array[0] = CLEAR_STAT_BUF;
	array[1] = PRE_MASK;
	array[2] = PRESCAN;
	array[3] = POST_MASK;
	array[4] = POSTSCAN;
	array[5] = PAR_SIZE;
	array[6] = SER_SIZE;
	array[7] = SEND_STAT_BUF;
	array[8] = 0;
	array[9] = 10;




	memset(&ioctl_val, '\0',sizeof(ioctl_wr_rd_type));
	ioctl_val.c_class    = '?';      /* These are the values that IOCTL    */
	ioctl_val.write_bytes= 10;  /*   needs to perform the single-call */
	ioctl_val.read_bytes =  10;  /*   write/read function.  We just    */
	ioctl_val.write_array= array;  /*   pass them through.               */
	ioctl_val.read_array =  outbuff;

	retcode = ioctl(3,IOCTL_WRITE_READ,(caddr_t)(&ioctl_val));
		printf("%d ", retcode);

}



