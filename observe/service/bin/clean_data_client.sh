#!/bin/bash

/usr/bin/find /data/client/photometric/ -mtime +14 -type f -exec rm {} \;
/usr/bin/find /data/client/pointing/ -mtime +14 -type f -exec rm {} \;
