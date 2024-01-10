#!/bin/bash

# $Date$
# $Rev$
# $URL$

FITS_FILE=$1
DIRNAME=$(dirname $FITS_FILE)

case "$DIRNAME" in
    "/data/CCD700.RAW/INCOMING" )
        DATA2M_PATH="ccd700"
        ;;
    "/data/CCD700.RAW/INCOMING/TECH" )
        DATA2M_PATH="ccd700_tech"
        ;;
    * )
        echo "$DESTINATION is forbidden path"
        exit 1
        ;;
esac

/usr/bin/rsync $FITS_FILE "data2m@ester.stel:${DATA2M_PATH}" >>/var/log/rsync.log 2>&1 &
/usr/bin/rsync $FITS_FILE "tcsuser@primula.stel:${DIRNAME}" >>/dev/null 2>&1 &

exit $?
