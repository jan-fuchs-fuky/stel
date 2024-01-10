/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 * $URL$
 */

#ifndef __MOD_CCD_SAURON_H
#define __MOD_CCD_SAURON_H

typedef enum
{
    SAURON_SPEED_100KHZ_E, SAURON_SPEED_1MHZ_E, SAURON_SPEED_MAX_E,
} SAURON_SPEED_E;

struct
{
    const char *str;
    SAURON_SPEED_E speed_e;
} sauron_speed_str2enum[] =
{
{ "100kHz", SAURON_SPEED_100KHZ_E },
{ "1MHz", SAURON_SPEED_1MHZ_E }, };

#endif
