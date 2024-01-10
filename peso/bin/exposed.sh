#!/bin/bash

export EXPOSED_DIR=$PWD
export LOG4C_RCPATH="$EXPOSED_DIR/etc"

./bin/exposed -c "$EXPOSED_DIR/share/exposed-dummy.ini"
