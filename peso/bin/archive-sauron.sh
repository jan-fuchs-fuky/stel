#!/bin/bash

# $Date$
# $Rev$
# $URL$

FITS_FILE=$1
DIRNAME=$(dirname $FITS_FILE)

case "$DIRNAME" in
    "/data/oes" )
        DATA2M_PATH="oes"
        ;;
    "/data/oes/TECH" )
        DATA2M_PATH="oes_tech"
        ;;
    * )
        echo "$DESTINATION is forbidden path"
        exit 1
        ;;
esac

/usr/bin/rsync $FITS_FILE "data2m@192.168.192.114:${DATA2M_PATH}" >>/var/log/rsync.log 2>&1 &

exit $?
