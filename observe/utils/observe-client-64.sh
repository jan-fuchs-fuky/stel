#!/bin/bash

SCRIPT_PATH="${BASH_SOURCE[0]}"

if [[ -h "${SCRIPT_PATH}" ]]; then
    while [[ -h "${SCRIPT_PATH}" ]]; do
        SCRIPT_PATH=$(readlink "${SCRIPT_PATH}")
    done
fi

pushd . >/dev/null

cd $(dirname "${SCRIPT_PATH}")
SCRIPT_PATH=$PWD

echo "SCRIPT_PATH='${SCRIPT_PATH}'"
./jre_linux_64/bin/java "-Dobserve.dir=$SCRIPT_PATH" -jar observe-client.jar

popd >/dev/null
