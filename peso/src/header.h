/*
 *  This file was autogenerated by make_header.py.
 *  DO NOT EDIT THIS FILE.
 *
 *  Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *  $Date$
 *  $Rev$
 */

#ifndef __HEADER_H
#define __HEADER_H

#define PHDR_KEY_MAX 8
#define PHDR_VALUE_MAX 68
#define PHDR_COMMENT_MAX 44

typedef enum {
    PHDR_TYPE_STR_E,
    PHDR_TYPE_INT_E,
    PHDR_TYPE_FLOAT_E,
    PHDR_TYPE_DOUBLE_E,
} PHDR_TYPE_T;

typedef enum {
    PHDR_ORIGIN_E,
    PHDR_OBSERVAT_E,
    PHDR_LATITUDE_E,
    PHDR_LONGITUD_E,
    PHDR_HEIGHT_E,
    PHDR_TELESCOP_E,
    PHDR_GAIN_E,
    PHDR_READNOIS_E,
    PHDR_TELSYST_E,
    PHDR_INSTRUME_E,
    PHDR_CAMERA_E,
    PHDR_DETECTOR_E,
    PHDR_CHIPID_E,
    PHDR_BUNIT_E,
    PHDR_PREFLASH_E,
    PHDR_CCDXSIZE_E,
    PHDR_CCDYSIZE_E,
    PHDR_CCDXPIXE_E,
    PHDR_CCDYPIXE_E,
    PHDR_DISPAXIS_E,
    PHDR_GRATNAME_E,
    PHDR_SLITTYPE_E,
    PHDR_AUTOGUID_E,
    PHDR_SLITWID_E,
    PHDR_COLIMAT_E,
    PHDR_TLE_TRCS_E,
    PHDR_TLE_TRGV_E,
    PHDR_TLE_TRHD_E,
    PHDR_TLE_TRRD_E,
    PHDR_TLE_TRUS_E,
    PHDR_SGH_MCO_E,
    PHDR_SGH_MSC_E,
    PHDR_SGH_CPA_E,
    PHDR_SGH_CPB_E,
    PHDR_SGH_OIC_E,
    PHDR_TM_DIFF_E,
    PHDR_OBJECT_E,
    PHDR_IMAGETYP_E,
    PHDR_OBSERVER_E,
    PHDR_SYSVER_E,
    PHDR_READSPD_E,
    PHDR_FILENAME_E,
    PHDR_CAMFOCUS_E,
    PHDR_SPECTEMP_E,
    PHDR_SPECFILT_E,
    PHDR_SLITHEIG_E,
    PHDR_TM_START_E,
    PHDR_UT_E,
    PHDR_EPOCH_E,
    PHDR_EQUINOX_E,
    PHDR_DATE_OBS_E,
    PHDR_TM_END_E,
    PHDR_EXPTIME_E,
    PHDR_DARKTIME_E,
    PHDR_CCDTEMP_E,
    PHDR_EXPVAL_E,
    PHDR_BIASSEC_E,
    PHDR_TRIMSEC_E,
    PHDR_GRATANG_E,
    PHDR_GRATPOS_E,
    PHDR_DICHMIR_E,
    PHDR_FLATTYPE_E,
    PHDR_COMPLAMP_E,
    PHDR_RA_E,
    PHDR_DEC_E,
    PHDR_ST_E,
    PHDR_TELFOCUS_E,
    PHDR_DOMEAZ_E,
    PHDR_AIRPRESS_E,
    PHDR_AIRHUMEX_E,
    PHDR_OUTTEMP_E,
    PHDR_DOMETEMP_E,
    PHDR_AMPLM_E,
    PHDR_CCDSUM_E,
    PHDR_CCDXIMSI_E,
    PHDR_CCDXIMST_E,
    PHDR_CCDYIMSI_E,
    PHDR_CCDYIMST_E,
    PHDR_DATAMAX_E,
    PHDR_DATAMIN_E,
    PHDR_GAINM_E,
    PHDR_MPP_E,
    PHDR_INDEX_MAX_E,
} PHDR_INDEX_T;

typedef struct {
    const char *key;
    PHDR_INDEX_T index;
    PHDR_TYPE_T type;
    char value[PHDR_VALUE_MAX+1];
    char comment[PHDR_COMMENT_MAX+1];
} PESO_HEADER_T;

#endif
