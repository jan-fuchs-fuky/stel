#!/bin/bash

CCD400_PATH="/zfs/nfs/2m/CCD400.RAW/INCOMING"
CCD400_TECH_PATH="/zfs/nfs/2m/CCD400.RAW/INCOMING/TECH"

CCD700_PATH="/zfs/nfs/2m/CCD700.RAW/INCOMING"
CCD700_NIGHT_PATH="/zfs/nfs/2m/CCD700.RAW"
CCD700_TECH_PATH="/zfs/nfs/2m/CCD700.RAW/INCOMING/TECH"

OES_PATH="/zfs/nfs/2m/OES/INCOMING"
OES_NIGHT_PATH="/zfs/nfs/2m/OES"
OES_TECH_PATH="/zfs/nfs/2m/OES/INCOMING/TECH"

DEST_PATH=${SSH_ORIGINAL_COMMAND##* }

ACTUAL_NIGHT=$(date +"%Y/%Y%m%d" -d "-12 hour")

if [ "$DEST_PATH" == "ccd400" ]; then
    DEST_PATH="$CCD400_PATH"
elif [ "$DEST_PATH" == "ccd700" ]; then
    DEST_PATH="$CCD700_PATH"
elif [ "$DEST_PATH" == "ccd700_night" ]; then
    DEST_PATH="${CCD700_NIGHT_PATH}/${ACTUAL_NIGHT}"
    mkdir -p "$DEST_PATH"
elif [ "$DEST_PATH" == "oes" ]; then
    DEST_PATH="$OES_PATH"
elif [ "$DEST_PATH" == "oes_night" ]; then
    DEST_PATH="${OES_NIGHT_PATH}/${ACTUAL_NIGHT}"
    mkdir -p "$DEST_PATH"
elif [ "$DEST_PATH" == "ccd400_tech" ]; then
    DEST_PATH="$CCD400_TECH_PATH"
elif [ "$DEST_PATH" == "ccd700_tech" ]; then
    DEST_PATH="$CCD700_TECH_PATH"
elif [ "$DEST_PATH" == "oes_tech" ]; then
    DEST_PATH="$OES_TECH_PATH"
else
    logger -p warn "WARNING: data2m_rsync rejected: $SSH_CONNECTION $SSH_ORIGINAL_COMMAND"
    echo "rejected" >/dev/stderr
    exit 1
fi

logger "data2m_rsync: $SSH_CONNECTION $SSH_ORIGINAL_COMMAND"

umask 0002
exec /usr/bin/rsync --server --chmod=ug+rwX,o+rX,o-w -e.Lsf . "$DEST_PATH"
