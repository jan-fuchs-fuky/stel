#!/bin/bash

# $Date: 2011-06-14 12:15:59 +0200 (Tue, 14 Jun 2011) $
# $Rev: 308 $
# $URL: https://stelweb.asu.cas.cz/svn/peso/trunk/bin/archive-sauron.sh $

FITS_FILE=$1
DIRNAME=$(dirname $FITS_FILE)

case "$DIRNAME" in
    "/data/OES/INCOMING" )
        DATA2M_PATH="oes_night"
        /usr/bin/rsync $FITS_FILE "data2m@sirius.stel:oes" >>/var/log/rsync.log 2>&1 &
        ;;
    "/data/OES/INCOMING/TECH" )
        DATA2M_PATH="oes_tech"
        ;;
    * )
        echo "$DESTINATION is forbidden path"
        exit 1
        ;;
esac

/usr/bin/rsync $FITS_FILE "data2m@sirius.stel:${DATA2M_PATH}" >>/var/log/rsync.log 2>&1 &
/usr/bin/rsync $FITS_FILE "tcsuser@primula.stel:${DIRNAME}" >>/dev/null 2>&1 &
/usr/bin/rsync $FITS_FILE "tcsuser@sulafat.stel:${DIRNAME}" >>/dev/null 2>&1 &
/usr/bin/rsync $FITS_FILE "tcsuser@nebula.stel:${DIRNAME}" >>/dev/null 2>&1 &

exit $?
