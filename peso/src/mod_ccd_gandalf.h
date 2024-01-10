/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 * $URL$
 */

#ifndef __MOD_CCD_GANDALF_H
#define __MOD_CCD_GANDALF_H

typedef enum
{
    GAN_SPEED_50KHZ_E = 50,
    GAN_SPEED_100KHZ_E = 100,
    GAN_SPEED_200KHZ_E = 200,
    GAN_SPEED_500KHZ_E = 500,
    GAN_SPEED_1MHZ_E = 1000,
    GAN_SPEED_2MHZ_E = 2000,
    GAN_SPEED_4MHZ_E = 4000,
    GAN_SPEED_MAX_E = 7
} GAN_SPEED_E;

typedef enum
{
    GAN_GAIN_LOW_E = 1,
    GAN_GAIN_MEDIUM_E = 2,
    GAN_GAIN_HIGH_E = 3,
    GAN_GAIN_MAX_E = 3
} GAN_GAIN_E;

// 4, 2, 1, 0.5, 0.2, 0.1, 0.05
struct
{
    const char *str;
    GAN_SPEED_E speed_e;
} gan_speed_str2enum[] =
{
    { "50kHz", GAN_SPEED_50KHZ_E },
    { "100kHz", GAN_SPEED_100KHZ_E },
    { "200kHz", GAN_SPEED_200KHZ_E },
    { "500kHz", GAN_SPEED_500KHZ_E },
    { "1MHz", GAN_SPEED_1MHZ_E },
    { "2MHz", GAN_SPEED_2MHZ_E },
    { "4MHz", GAN_SPEED_4MHZ_E },
};

struct
{
    const char *str;
    GAN_GAIN_E gain_e;
} gan_gain_str2enum[] =
{
    { "low", GAN_GAIN_LOW_E },
    { "medium", GAN_GAIN_MEDIUM_E },
    { "high", GAN_GAIN_HIGH_E },
};

#endif
