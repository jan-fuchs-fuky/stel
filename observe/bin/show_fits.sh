#!/bin/bash

FITS=$1

FITS_VERIFY_CHKSUM="/opt/bin/fits_verify_chksum"

CCD=${FITS##/data/}
CCD=${CCD%%/*}
CCD=${CCD%%.*}

COUDE=1
case "$(hostname)" in
    primula )
        ;;
    almisan )
        ;;
    sulafat )
        ;;
    * )
        COUDE=0
        FITS="/i/data2m/${FITS##/data/}"
        ;;
esac

echo $CCD
echo $FITS

RUN_DS9=0

for i in $(seq 100); do
    if [ -e "$FITS" ]; then

        $FITS_VERIFY_CHKSUM $FITS
        if [ $? -eq 0 ]; then

            xpaget "$CCD" mode
            if [ $? -ne 0 ]; then

                RUN_DS9=1
                nohup ds9 \
                    -title "$CCD" \
                    -scale log \
                    -view graph horizontal yes \
                    -view graph vertical yes \
                    -mode none \
                    -view minmax yes \
                    -view wcs no \
                    -geometry 1024x768 >/dev/null 2>&1 &

                for i in $(seq 50); do
                    xpaget "$CCD" mode
                    if [ $? -eq 0 ]; then
                        break
                    fi

                    if [ $i -eq 50 ]; then
                        echo "ERROR: DS9 ${CCD} not found"
                        exit
                    fi

                    sleep 0.1
                done
            fi

            ERR_MSG=$(xpaset -p "$CCD" file "$FITS")
            if [ $? -ne 0 ] && [ "$COUDE" -eq 1 ]; then
                mutt -s "ERR: show_fits.sh" "fuky@asu.cas.cz" <<EOF
$ERR_MSG
EOF
            fi

            #if [ "$RUN_DS9" -eq 1 ]; then
                xpaset -p "$CCD" zoom to fit
            #fi

            beep -f 1000 -n -f 2000 -n -f 3000
            exit
        fi
    fi
    sleep 0.1
done
