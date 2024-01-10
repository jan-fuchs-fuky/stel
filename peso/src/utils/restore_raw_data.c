#include <stdio.h>
#include <stdlib.h>
#include <fitsio.h>
#include <glib.h>
#include <string.h>

#include "../header.h"

#define BUFFER_MAX 1023

extern PESO_HEADER_T peso_header[];

// TODO: nacitat z hlavicky
static long naxis1 = 2048, naxis2 = 512;

// FROM: exposed.c
__attribute__((format(printf,2,3)))
static int save_fits_error(int fits_status, const char *p_fmt, ...)
{
    va_list ap;
    char msg[BUFFER_MAX+1];
    char fits_error[FLEN_ERRMSG];

    va_start(ap, p_fmt);

    memset(msg, '\0', BUFFER_MAX+1);
    vsnprintf(msg, BUFFER_MAX, p_fmt, ap);

    va_end(ap);

    fits_get_errstatus(fits_status, fits_error);
    printf("%s %s\n", msg, fits_error);
    return 0;
}

// FROM: exposed.c
static long save_fits_header(fitsfile *p_fits)
{
    int i;
    int fits_status = 0;
    int bscale = 1;
    int bzero = 32768;
    int value_int;
    float value_float;
    double value_double;
    char key[PHDR_KEY_MAX + 1];
    char *p_value;
    char *p_comment;
    long naxis = 2;
    long naxes[2] = { naxis1, naxis2 };

    if (fits_create_img(p_fits, USHORT_IMG, naxis, naxes, &fits_status))
    {
        save_fits_error(fits_status,
                "Error: fits_create_img(USHORT_IMG, %li, [%li, %li]):", naxis,
                naxes[0], naxes[1]);
        return -1;
    }
    if (fits_update_key(p_fits, TINT, "BZERO", &bzero, "", &fits_status))
    {
        save_fits_error(fits_status, "Error: fits_update_key(bzero = %i):",
                bzero);
        return -1;
    }
    if (fits_update_key(p_fits, TINT, "BSCALE", &bscale,
            "REAL=TAPE*BSCALE+BZERO", &fits_status))
    {
        save_fits_error(fits_status, "Error: fits_update_key(bscale = %i):",
                bscale);
        return -1;
    }

    for (i = 0; i < PHDR_INDEX_MAX_E; ++i)
    {
        if (peso_header[i].value[0] == '\0')
        {
            continue;
        }

        memset(key, '\0', PHDR_KEY_MAX + 1);
        strncpy(key, peso_header[i].key, PHDR_KEY_MAX);
        p_value = peso_header[i].value;
        p_comment = peso_header[i].comment;

        switch (peso_header[i].type)
        {
        case PHDR_TYPE_INT_E:
            value_int = atoi(p_value);
            if (fits_update_key(p_fits, TINT, key, &value_int, p_comment,
                    &fits_status))
            {
                save_fits_error(fits_status,
                        "Error: fits_update_key(TLONG, %s, %i, %s):", key,
                        value_int, p_comment);
                return -1;
            }
            break;

        case PHDR_TYPE_FLOAT_E:
            value_float = atof(p_value);
            if (fits_update_key(p_fits, TFLOAT, key, &value_float, p_comment,
                    &fits_status))
            {
                save_fits_error(fits_status,
                        "Error: fits_update_key(TFLOAT, %s, %0.2f, %s):", key,
                        value_float, p_comment);
                return -1;
            }
            break;

        case PHDR_TYPE_DOUBLE_E:
            value_double = atof(p_value);
            if (fits_update_key(p_fits, TDOUBLE, key, &value_double, p_comment,
                    &fits_status))
            {
                save_fits_error(fits_status,
                        "Error: fits_update_key(TDOUBLE, %s, %0.2f, %s):", key,
                        value_double, p_comment);
                return -1;
            }
            break;

        case PHDR_TYPE_STR_E:
            if (fits_update_key(p_fits, TSTRING, key, p_value, p_comment,
                    &fits_status))
            {
                save_fits_error(fits_status,
                        "Error: fits_update_key(TSTRING, %s, %s, %s):", key,
                        p_value, p_comment);
                return -1;
            }
            break;

        default:
            break;
        }
    }

    if (fits_write_date(p_fits, &fits_status))
    {
        save_fits_error(fits_status, "Error: fits_write_date():");
        return -1;
    }

    return 0;
}

static int next_hdr_item(FILE *p_fr)
{
    char c;
    char line[BUFFER_MAX+1];
    char key[BUFFER_MAX+1];
    char value[BUFFER_MAX+1];
    char comment[BUFFER_MAX+1];
    char *p_value;
    char *p_comment;
    int idx = 0;
    int i;
    int section = 0;
  
    memset(line, 0, sizeof(line));
    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));
    memset(comment, 0, sizeof(comment));
  
    if (fgets(line, BUFFER_MAX, p_fr) == NULL) {
        return -1;
    }
    else {
        for (i = 0; line[i] != '\0'; ++i) {
            c = line[i];

            if (c == '\n') {
                break;
            }
            else if (((section == 0) && (c == '=')) || ((section == 1) && (c == '/'))) {
                idx = 0;
                ++section;
            }
            else {
                switch (section) {
                    case 0:
                        if (c != ' ') {
                            key[idx++] = c;
                        }
                        break;

                    case 1:
                        value[idx++] = c;
                        break;

                    case 2:
                    default:
                        comment[idx++] = c;
                        break;
                }
            }

            if (idx > (BUFFER_MAX-1)) {
                printf("ERROR: idx > BUFFER_MAX-1 - %s\n", line);
                exit(EXIT_SUCCESS);
            }

            if (section >= 3) {
                printf("ERROR: section >= 3 - %s\n", line);
                exit(EXIT_SUCCESS);
            }
        }
    }

    p_value = g_strstrip(value);
    p_comment = g_strstrip(comment);

    printf("'%s' = '%s' / '%s'\n", key, p_value, p_comment);

    for (i = 0; i < PHDR_INDEX_MAX_E; ++i) {
        if (!strcmp(peso_header[i].key, key)) {
            snprintf(peso_header[i].value, PHDR_VALUE_MAX, "%s", p_value);

            if (comment[0] != '\0') {
                snprintf(peso_header[i].comment, PHDR_COMMENT_MAX, "%s", p_comment);
            }
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    fitsfile *p_fitsfile;
    int fits_status = 0; // MUST initialize fits_status
    FILE *p_fr = NULL;
    long fpixel = 1;
    long nelements;
    static unsigned short *p_raw_data = NULL;

    if (argc != 4) {
        printf("Usage: %s DATA_RAW DATA_HDR DATA_FIT", argv[0]);
        exit(EXIT_SUCCESS);
    }

    // Read header
    if ((p_fr = fopen(argv[2], "r")) == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    while (next_hdr_item(p_fr) != -1);

    fclose(p_fr);

    // TODO
    //naxis1 = atoi(peso_header[PHDR_NAXIS1_E].value);
    //naxis2 = atoi(peso_header[PHDR_NAXIS2_E].value);
    nelements = naxis1 * naxis2;
    
    if ((p_raw_data = (unsigned short *) malloc(nelements*2)) == NULL) {
        perror("malloc(): ");
        exit(EXIT_FAILURE);
    }

    // Read data
    if ((p_fr = fopen(argv[1], "r")) == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    fread(p_raw_data, sizeof(*p_raw_data), nelements, p_fr);

    fclose(p_fr);

    fits_create_file(&p_fitsfile, argv[3], &fits_status);
    if (fits_status) {
        fits_report_error(stderr, fits_status);
        exit(fits_status);
    }

    save_fits_header(p_fitsfile);

    if (fits_write_img(p_fitsfile, TUSHORT, fpixel, nelements, p_raw_data, &fits_status)) {
        fits_report_error(stderr, fits_status);
        exit(fits_status);
    }

    fits_write_chksum(p_fitsfile, &fits_status);
    fits_close_file(p_fitsfile, &fits_status);

    free(p_raw_data);
    exit(EXIT_SUCCESS);
}
