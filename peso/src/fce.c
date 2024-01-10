/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 * $URL$
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "cfg.h"
#include "fce.h"

#ifdef SELF_TEST_FCE

#include <log4c.h>

#define PREFIX_MAX 1023

time_t st_fce_time;
log4c_category_t *p_logcat = NULL;

#endif

/*
 * https://stelweb.asu.cas.cz/wiki/index.php/2m_FITS
 *
 *    Prefix is iYYYYMMDD (full filename is iYYYYMMDDNNNN.fit):
 *    
 *        i = konfigurace
 *            a = 700mm + PyLoN 2048x512BX
 *            b = 400mm + STA0520A
 *            c = OES + EEV 42-40-1-368 
 *        YYYY = rok
 *        MM = mesic
 *        DD = den
 *        NNNN = cislo snimku 
 *    
 *    Maximalni delku nazvu nechame 18 znaku, kvuli FITS hlavicce. 
 *
 * DEPRECATED from 23.5. 2013:
 *
 *    Prefix is xrb08:
 *
 *      x   = instrument prefix ('a' - OES, 'c' - CCD400, ' ' - CCD700)
 *      r   = year 2008
 *      b   = month February (02)
 *      08  = 8. day
 *
 *    Year:
 *
 *        2008 = 'r'
 *        2009 = 's'
 *        2010 = 't'
 *        2011 = 'u'
 *        2012 = 'v'
 *        2013 = 'w'
 *        2014 = 'x'
 *        2015 = 'y'
 *        2016 = 'z'
 *
 *    Month:
 *
 *        01 = 'a'
 *        02 = 'b'
 *        03 = 'c'
 *        04 = 'd'
 *        05 = 'e'
 *        06 = 'f'
 *        07 = 'g'
 *        08 = 'h'
 *        09 = 'i'
 *        10 = 'j'
 *        11 = 'k'
 *        12 = 'l'
 */
int fce_make_fits_prefix(char instrument_prefix, char *p_prefix, int prefix_len)
{
    //int index = 0;
    time_t t;
    struct tm local_tm;

    /* assert */
    if (prefix_len < 9)
    {
        return -1;
    }

#ifdef SELF_TEST_FCE
    t = st_fce_time;
#else
    t = time(NULL);
#endif

    // For portable code tzset(3) should be called before localtime_r().
    tzset();

    t -= (12 * 3600); // 12 hours
    localtime_r(&t, &local_tm);
    memset(p_prefix, 0, prefix_len);

#ifdef SELF_TEST_FCE
    printf("%04i-%02i-%02i %02i:%02i:%02i\n", local_tm.tm_year+1900, local_tm.tm_mon+1, local_tm.tm_mday,
        local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);
    printf("Offset to GMT is %lds.\n", local_tm.tm_gmtoff);
#endif

    // iYYYYMMDD
    snprintf(p_prefix, prefix_len, "%c%04i%02i%02i", instrument_prefix,
        local_tm.tm_year+1900, local_tm.tm_mon+1, local_tm.tm_mday);

    // DEPRECATED begin
    //if ((instrument_prefix != ' ') && (instrument_prefix != '\0'))
    //{
    //    p_prefix[index++] = instrument_prefix;
    //}

    //switch (p_tm->tm_year)
    //{
    //case 108:
    //    p_prefix[index++] = 'r';
    //    break;
    //case 109:
    //    p_prefix[index++] = 's';
    //    break;
    //case 110:
    //    p_prefix[index++] = 't';
    //    break;
    //case 111:
    //    p_prefix[index++] = 'u';
    //    break;
    //case 112:
    //    p_prefix[index++] = 'v';
    //    break;
    //case 113:
    //    p_prefix[index++] = 'w';
    //    break;
    //case 114:
    //    p_prefix[index++] = 'x';
    //    break;
    //case 115:
    //    p_prefix[index++] = 'y';
    //    break;
    //case 116:
    //    p_prefix[index++] = 'z';
    //    break;
    //default:
    //    break;
    //}

    //switch (p_tm->tm_mon)
    //{
    //case 0:
    //    p_prefix[index++] = 'a';
    //    break;
    //case 1:
    //    p_prefix[index++] = 'b';
    //    break;
    //case 2:
    //    p_prefix[index++] = 'c';
    //    break;
    //case 3:
    //    p_prefix[index++] = 'd';
    //    break;
    //case 4:
    //    p_prefix[index++] = 'e';
    //    break;
    //case 5:
    //    p_prefix[index++] = 'f';
    //    break;
    //case 6:
    //    p_prefix[index++] = 'g';
    //    break;
    //case 7:
    //    p_prefix[index++] = 'h';
    //    break;
    //case 8:
    //    p_prefix[index++] = 'i';
    //    break;
    //case 9:
    //    p_prefix[index++] = 'j';
    //    break;
    //case 10:
    //    p_prefix[index++] = 'k';
    //    break;
    //case 11:
    //    p_prefix[index++] = 'l';
    //    break;
    //default:
    //    break;
    //}
    // DEPRECATED end

    return 0;
}

int fce_make_filename(char *p_path, char *p_prefix, char *p_filename,
        int filename_len)
{
    int i;
    int j;
    int len = 0;
    char *suffix[] =
    { "raw", "fit" };

    for (i = 1; i <= PESO_MAX_NUMBER_OF_EXPOSE; ++i)
    {
        for (j = 0; j < 2; ++j)
        {
            len = snprintf(p_filename, filename_len, "%s/%s%04i.%s", p_path,
                    p_prefix, i, suffix[j]);

            /* assert */
            if (len > (filename_len - 1))
            {
                i = PESO_MAX_NUMBER_OF_EXPOSE;
                break;
            }

            if (fce_isfile_exist(p_filename))
            {
                break;
            }
            else if (j == 1)
            {
                p_filename[strlen(p_filename) - 3] = '\0'; /* remove suffix */
                return 0;
            }
        }
    }

    memset(p_filename, '\0', filename_len);
    return -1;
}

int fce_isfile_exist(char *p_filename)
{
    FILE *p_fr;

    if ((p_fr = fopen(p_filename, "r")) == NULL)
    {
        return 0;
    }
    else
    {
        fclose(p_fr);
        return 1;
    }
}

int fce_isdir_rwxu(char *p_path)
{
    struct stat sb;

    // TODO: zalogovat chybu
    if (stat(p_path, &sb) == -1)
    {
        return 0;
    }

    if (S_ISDIR(sb.st_mode))
    {
        if (sb.st_uid == exposed_cfg.uid)
        {
            if ((sb.st_mode & S_IRWXU) == (S_IRUSR | S_IWUSR | S_IXUSR))
            {
                return 1;
            }
        }
        else if (sb.st_gid == exposed_cfg.gid)
        {
            if ((sb.st_mode & S_IRWXG) == (S_IRGRP | S_IWGRP | S_IXGRP))
            {
                return 1;
            }
        }
        else
        {
            if ((sb.st_mode & S_IRWXO) == (S_IROTH | S_IWOTH | S_IXOTH))
            {
                return 1;
            }
        }
    }

    return 0;
}

int fce_isallow_fitshdr_c(int c)
{
    int i;
    int allow_chars_len;
    char allow_chars[] = " <>{}()[]+-_=~!@#$%^&*|'\";:.?";

    /* a-z */
    if ((c >= 'a') && (c <= 'z'))
    {
        return 1;
    }

    /* A-Z */
    if ((c >= 'A') && (c <= 'Z'))
    {
        return 1;
    }

    /* 0-9 */
    if ((c >= '0') && (c <= '9'))
    {
        return 1;
    }

    allow_chars_len = strlen(allow_chars);
    for (i = 0; i < allow_chars_len; i++)
    {
        if (c == allow_chars[i])
        {
            return 1;
        }
    }

    return 0;
}

char *fce_strstrip(char *p_str)
{
    char *p_begin;
    char *p_end;
    int i;
    int len = strlen(p_str);

    if (len == 0)
    {
        return p_str;
    }

    p_begin = p_str;
    p_end = p_begin + len - 1;

    for (i = 0; i < len; ++i)
    {
        if (*p_begin == ' ')
        {
            p_begin++;
        }
        else
        {
            break;
        }
    }

    for (i = len; i > 0; --i)
    {
        if (*p_end == ' ')
        {
            *p_end = '\0';
            p_end--;
        }
        else
        {
            break;
        }
    }

    return p_begin;
}

#ifdef SELF_TEST_FCE

int main(int argc, char *argv[])
{
    int i;
    char prefix[PREFIX_MAX+1];

    // 2013-09-30 12:00
    for (i = 1; i >= 0; --i) {
        // UT - 2h - i
        st_fce_time = 1380542400 - (2 * 3600) - i;
        fce_make_fits_prefix('a', prefix, PREFIX_MAX);
        printf("fce_make_fits_prefix(time = %li) => prefix = %s\n", st_fce_time, prefix);
    }

    // 2013-10-01 12:00
    for (i = 1; i >= 0; --i) {
        // UT - 2h - i
        st_fce_time = 1380628800 - (2 * 3600) - i;
        fce_make_fits_prefix('a', prefix, PREFIX_MAX);
        printf("fce_make_fits_prefix(time = %li) => prefix = %s\n", st_fce_time, prefix);
    }

    // 2013-11-10 12:00
    for (i = 1; i >= 0; --i) {
        // UT - 1h - i
        st_fce_time = 1384084800 - 3600 - i;
        fce_make_fits_prefix('a', prefix, PREFIX_MAX);
        printf("fce_make_fits_prefix(time = %li) => prefix = %s\n", st_fce_time, prefix);
    }

    exit(EXIT_SUCCESS);
}
#endif
