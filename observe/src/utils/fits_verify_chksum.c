#include <stdio.h>
#include <stdlib.h>
#include <fitsio.h>

int main(int argc, char *argv[])
{
    fitsfile *p_fitsfile;
    int status = 0; // MUST initialize status
    int data_ok;
    int hdu_ok;

    if (argc != 2) {
        printf("Usage: %s FILENAME_FITS", argv[0]);
        exit(EXIT_SUCCESS);
    }

    fits_open_file(&p_fitsfile, argv[1], READONLY, &status);
    if (status) {
        fits_report_error(stderr, status);
        exit(status);
    }

    fits_verify_chksum(p_fitsfile, &data_ok, &hdu_ok, &status);
    if (status) {
        fits_report_error(stderr, status);
        exit(status);
    }

    fits_close_file(p_fitsfile, &status);

    printf("%i %i\n", data_ok, hdu_ok);

    if ((data_ok == 1) && (hdu_ok == 1)) {
        exit(EXIT_SUCCESS);
    }
    else {
        exit(EXIT_FAILURE);
    }
}
