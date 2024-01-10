#!/usr/bin/python3

#
# Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
# $Date$
# $Rev$
#

HEADER_KEY = 0
HEADER_VALUE = 1
HEADER_COMMENT = 2
HEADER_TYPE = 3

# key, value, comment, type
header_keys = []
header_keys.append(["ORIGIN"   , "\\0", "AsU AV CR Ondrejov",                         "str"])
header_keys.append(["OBSERVAT" , "\\0", "Name of observatory (IRAF style)",           "str"])
header_keys.append(["LATITUDE" , "\\0", "Telescope latitude  (degrees), +49:54:38.0", "float"])
header_keys.append(["LONGITUD" , "\\0", "Telescope longitud  (degrees), +14:47:01.0", "float"])
header_keys.append(["HEIGHT"   , "\\0", "Height above sea level [m].",                "int"])  
header_keys.append(["TELESCOP" , "\\0", "2m Ondrejov observatory telescope",          "str"])  
header_keys.append(["GAIN"     , "\\0", "Electrons per ADU",                          "int"])  
header_keys.append(["READNOIS" , "\\0", "Readout noise in electrons per pix",         "int"])  
header_keys.append(["TELSYST"  , "\\0", "Telescope setup - COUDE or CASSegrain",      "str"])  
header_keys.append(["INSTRUME" , "\\0", "Coude echelle spectrograph",                 "str"])  
header_keys.append(["CAMERA"   , "\\0", "Camera head name",                           "str"])  
header_keys.append(["DETECTOR" , "\\0", "Name of the detector",                       "str"])  
header_keys.append(["CHIPID"   , "\\0", "Name of CCD chip",                           "str"])
header_keys.append(["BUNIT"    , "\\0", "Unit of the array of image data",            "str"])
header_keys.append(["PREFLASH" , "\\0", "Length of preflash in seconds",              "int"])
header_keys.append(["CCDXSIZE" , "\\0", "X Size in pixels of digitised frame",        "int"])
header_keys.append(["CCDYSIZE" , "\\0", "Y Size in pixels of digitised frame",        "int"])
header_keys.append(["CCDXPIXE" , "\\0", "Size in microns of the pixels, in X",        "float"])
header_keys.append(["CCDYPIXE" , "\\0", "Size in microns of the pixels, in Y",        "float"])
header_keys.append(["DISPAXIS" , "\\0", "Dispersion axis along lines",                "int"])
header_keys.append(["GRATNAME" , "\\0", "Grating name - ID",                          "str"])
header_keys.append(["SLITTYPE" , "\\0", "Type of slit - blade or image slicers",      "str"])
header_keys.append(["AUTOGUID" , "\\0", "Status of autoguider system",                "str"])
header_keys.append(["SLITWID"  , "\\0", "Slit width in mm",                           "float"])

header_keys.append(["COLIMAT"  , "\\0", "Collimator mask status", "str"])
header_keys.append(["TLE-TRCS" , "\\0", "Correction Set", "str"])
header_keys.append(["TLE-TRGV" , "\\0", "Guiding Value", "str"])
header_keys.append(["TLE-TRHD" , "\\0", "Hour and Declination Axis", "str"])
header_keys.append(["TLE-TRRD" , "\\0", "Right ascension and Declination", "str"])
header_keys.append(["TLE-TRUS" , "\\0", "User Speed", "str"])
header_keys.append(["SGH-MCO"  , "\\0", "Mirror Coude Oes", "str"])
header_keys.append(["SGH-MSC"  , "\\0", "Mirror Star Calibration", "str"])
header_keys.append(["SGH-CPA"  , "\\0", "Correction plate 700", "str"])
header_keys.append(["SGH-CPB"  , "\\0", "Correction plate 400", "str"])
header_keys.append(["SGH-OIC"   , "\\0", "OES Iodine cell", "int"])
header_keys.append(["TM-DIFF"  , "\\0", "\\0", "int"])

header_keys.append(["OBJECT"   , "\\0", "Title of observation", "str"])
header_keys.append(["IMAGETYP" , "\\0", "Type of observation, eg. FLAT", "str"])
header_keys.append(["OBSERVER" , "\\0", "Observers", "str"])
header_keys.append(["SYSVER"   , "\\0", "\\0", "str"])
header_keys.append(["READSPD"  , "\\0", "\\0", "str"])
header_keys.append(["FILENAME" , "\\0", "\\0", "str"])
header_keys.append(["CAMFOCUS" , "\\0", "Camera focus position", "float"])
header_keys.append(["SPECTEMP" , "\\0", "Temperature in spectrograph room", "float"])
header_keys.append(["SPECFILT" , "\\0", "Spectral filter", "int"])
header_keys.append(["SLITHEIG" , "\\0", "Slit hight in mm", "float"])
header_keys.append(["TM_START" , "\\0", "\\0", "int"])
header_keys.append(["UT"       , "\\0", "UTC of  start of observation", "str"])
header_keys.append(["EPOCH"    , "\\0", "Same as EQUINOX - for back compat", "double"])
header_keys.append(["EQUINOX"  , "\\0", "Equinox of RA and DEC", "double"])
header_keys.append(["DATE-OBS" , "\\0", "UTC date start of observation", "str"])
header_keys.append(["TM_END"   , "\\0", "\\0", "int"])
header_keys.append(["EXPTIME"  , "\\0", "Length of observation excluding pauses", "int"])
header_keys.append(["DARKTIME" , "\\0", "Length of observation including pauses", "int"])
header_keys.append(["CCDTEMP"  , "\\0", "Detector temperature", "int"])
header_keys.append(["EXPVAL"   , "\\0", "Exposure value in photon counts [Mcounts]", "float"])

header_keys.append(["BIASSEC"  , "\\0", "Overscan portion of frame", "str"])
header_keys.append(["TRIMSEC"  , "\\0", "Region to be extracted", "str"])
header_keys.append(["GRATANG"  , "\\0", "\\0", "float"])
header_keys.append(["GRATPOS"  , "\\0", "Grating angle in increments", "int"])
header_keys.append(["DICHMIR"  , "\\0", "Dichroic mirror number", "int"])
header_keys.append(["FLATTYPE" , "\\0", "Flat type (Projector/Dome)", "str"])
header_keys.append(["COMPLAMP" , "\\0", "Comparison arc setup", "str"])
header_keys.append(["RA"       , "\\0", "\\0", "str"])
header_keys.append(["DEC"      , "\\0", "\\0", "str"])
header_keys.append(["ST"       , "\\0", "Local sidereal time at start of observation", "str"])
header_keys.append(["TELFOCUS" , "\\0", "Telescope focus (milimeters)", "float"])
header_keys.append(["DOMEAZ"   , "\\0", "Mean dome azimuth during observation", "float"])
header_keys.append(["AIRPRESS" , "\\0", "Atmospheric preasure in (hPa)", "float"])
header_keys.append(["AIRHUMEX" , "\\0", "Air humidity outside the dome", "float"])
header_keys.append(["OUTTEMP"  , "\\0", "Temperature outside of the dome", "float"])
header_keys.append(["DOMETEMP" , "\\0", "Temperature inside the dome", "float"])

header_keys.append(["AMPLM" , "\\0", "Amplifier A,B or AB", "str"])
header_keys.append(["CCDSUM" , "\\0", "CCD binning in both axes", "str"])
header_keys.append(["CCDXIMSI" , "\\0", "X Size of useful imaging area", "int"])
header_keys.append(["CCDXIMST" , "\\0", "X Start pixel of useful imaging area", "int"])
header_keys.append(["CCDYIMSI" , "\\0", "Y Size of useful imaging area", "int"])
header_keys.append(["CCDYIMST" , "\\0", "Y Start pixel of useful imaging area", "int"])
header_keys.append(["DATAMAX" , "\\0", "DATA MAX", "int"])
header_keys.append(["DATAMIN" , "\\0", "DATA MIN", "int"])
header_keys.append(["GAINM" , "\\0", "Gain mode", "str"])
header_keys.append(["MPP" , "\\0", "Multiphase pinned mode (T/F)", "str"])

header = """\
/*
 *  This file was autogenerated by make_header.py.
 *  DO NOT EDIT THIS FILE.
 *
 *  Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *  $Date$
 *  $Rev$
 */

"""

# header.h begin
file_header_h = open("./src/header.h", "w")
file_header_h.write(header)

file_header_h.write("""\
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
""")

for items in header_keys:
    index = items[HEADER_KEY].replace("-", "_")
    file_header_h.write("    PHDR_%s_E,\n" % (index))

file_header_h.write("""\
    PHDR_INDEX_MAX_E,
} PHDR_INDEX_T;

typedef struct {
    const char *key;
    PHDR_INDEX_T index;
    PHDR_TYPE_T type;
    char value[PHDR_VALUE_MAX+1];
    char comment[PHDR_COMMENT_MAX+1];
} PESO_HEADER_T;

#endif\n""")
file_header_h.close()
# header.h end

# header.c begin
file_header_c = open("./src/header.c", "w")
file_header_c.write(header)
file_header_c.write('#include "header.h"\n\nPESO_HEADER_T peso_header[] = {\n')

for items in header_keys:
    key = items[HEADER_KEY]
    index = items[HEADER_KEY].replace("-", "_")
    key_type = items[HEADER_TYPE]

    if (key_type == "str"):
        key_type = "PHDR_TYPE_STR_E";
    elif (key_type == "int"):
        key_type = "PHDR_TYPE_INT_E";
    elif (key_type == "float"):
        key_type = "PHDR_TYPE_FLOAT_E";
    elif (key_type == "double"):
        key_type = "PHDR_TYPE_DOUBLE_E";

    file_header_c.write('    { "%s", PHDR_%s_E, %s, "%s", "%s" },\n' % \
        (key, index, key_type, items[HEADER_VALUE], items[HEADER_COMMENT]))

file_header_c.write("};\n")
file_header_c.close()
# header.c end
