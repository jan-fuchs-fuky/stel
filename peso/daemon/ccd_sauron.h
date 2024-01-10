#include <stdio.h>

#ifndef __CCD_SAURON_H
#define __CCD_SAURON_H

#define READOUT_TIME_10KHZ   41
#define READOUT_TIME_1MHZ    5

#define BUFFER_MAX   512
#define CMD_MAX      128

#define FILE_CCD_ADD_TIME           "../temp/ccd_sauron.add_time"
#define FILE_SOCKET                 "../sockets/ccd_sauron"
#define FILE_FIT_HEADER_TEMPLATE    "../share/comment.header"
#define FILE_FIT_HEADER             "../temp/fit.header"
#define FILE_COMMENT1               "../temp/comment1"
#define FILE_COMMENT2               "../temp/comment2"

#define LOG_MSG_MAX 256

#define KEY_MAX        12
#define VALUE_MAX      128
#define COMMENT_MAX    128
#define TYPE_MAX       12
#define KVCT_MAX       KEY_MAX+VALUE_MAX+COMMENT_MAX+TYPE_MAX+6

#define LOG_PVCAM_ERR      1001
#define LOG_HIDDEN_INFO    1002
#define LOG_PESO_INFO      1003

#define hmv2v(h,m,v) \
    ((h)*3600 + (m)*60 + (v))

/* http://aa.usno.navy.mil/faq/docs/JD_Formula.html */
#define gregorian2julian(i,j,k)                         \
    ((k)-32075+1461*((i)+4800+((j)-14)/12)/4+367*((j)-2-((j)-14)/12*12)/12-3*(((i)+4900+((j)-14)/12)/100)/4)

#define equinox(i) \
    (2000.0 + ((i) - 2451545.0) / 365.25)

typedef struct {
    char output[FILENAME_MAX];
    char cmd_before[CMD_MAX];
    char cmd_after[CMD_MAX];
    int x1;
    int x2;
    int xb;
    int y1;
    int y2;
    int yb;
    int time;
    int readout_time;
    int16 speed;
    int16 shutter;
    int16 temp;
} CCD_OPTION_T;

#endif
