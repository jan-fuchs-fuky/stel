#!/bin/bash

export TELESCOPE_HOST="localhost"
export TELESCOPE_PORT="9999"
export SPECTROGRAPH_HOST="localhost"
export SPECTROGRAPH_PORT="8888"

./bin/exposed -c ./etc/exposed-dummy.cfg
