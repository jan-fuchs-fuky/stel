#!/bin/bash

DEST_PATH=${SSH_ORIGINAL_COMMAND##* }

if [ "$DEST_PATH" == "pointing/" ]; then
    DEST_PATH="/zfs/nfs/2m/pointing/INCOMING"
elif [ "$DEST_PATH" == "fotometrie/" ]; then
    DEST_PATH="/zfs/nfs/2m/fotometrie/INCOMING"
else
    logger -p warn "WARNING: lsyncd_rsync.sh rejected: $SSH_CONNECTION $SSH_ORIGINAL_COMMAND"
    echo "rejected" >/dev/stderr
    exit 1
fi

logger "lsyncd_rsync.sh: $SSH_CONNECTION $SSH_ORIGINAL_COMMAND"
exec rsync --server -logDtpre.iLsfxC . "$DEST_PATH"
