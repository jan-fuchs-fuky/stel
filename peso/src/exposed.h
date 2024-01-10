/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 */

#ifndef __EXPOSED_H
#define __EXPOSED_H

#define APP_NAME         "exposed"
#define MAKE_DATE_TIME   "(" __DATE__ " " __TIME__ ")"

#define BUFFER_MAX              127
#define RESULT_MAX              127
#define COMMAND_MAX             127
#define EXPOSED_LOG_MAX         255
#define CIRCULAR_BUFFER_SIZE    512
#define EXPOSED_MOD_CCD_PREFIX  "mod_ccd_"
#define EXPOSED_CCD_NAME_MAX    31
#define EXPOSED_XML_MAX         1023
#define PREFIX_MAX              31
#define EXPOSED_STR_MAX         1023

#define hms2s(h,m,s) \
  ((h)*3600 + (m)*60 + (s))

/* http://aa.usno.navy.mil/faq/docs/JD_Formula.html */
#define gregorian2julian(i,j,k)             \
  ((k)-32075+1461*((i)+4800+((j)-14)/12)/4+367*((j)-2-((j)-14)/12*12)/12-3*(((i)+4900+((j)-14)/12)/100)/4)

#define equinox(i) \
  (2000.0 + ((i) - 2451545.0) / 365.25)

typedef struct
{
    int expose_sem;
    int service_sem;
    int global_mutex;
    int mod_ccd;
} EXPOSED_ALLOCATE_T;

extern log4c_category_t *p_logcat;

#endif
