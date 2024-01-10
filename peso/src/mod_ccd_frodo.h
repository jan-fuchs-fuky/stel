/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 * $URL$
 */

#ifndef __MOD_CCD_FRODO_H
#define __MOD_CCD_FRODO_H

#define FRO_HARDWARE_DATA_MAX 1000000

typedef enum
{
    FRO_SPEED_10KHZ_E, FRO_SPEED_1MHZ_E, FRO_SPEED_MAX_E,
} FRO_SPEED_E;

struct
{
    const char *str;
    FRO_SPEED_E speed_e;
} fro_speed_str2enum[] =
{
{ "10kHz", FRO_SPEED_10KHZ_E },
{ "1MHz", FRO_SPEED_1MHZ_E }, };

#endif
