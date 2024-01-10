#!/bin/bash

# version 1.0.1

case "$SSH_ORIGINAL_COMMAND" in
    rsync\ --server\ --daemon\ \.)
        $SSH_ORIGINAL_COMMAND
    ;;
    *)
        echo "Rejected"
    ;;
esac

