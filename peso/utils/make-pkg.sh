#!/bin/bash

SVN_REV=$(svnversion -n ..)
PKG_NAME="exposed-r${SVN_REV}"
PREFIX="../dist/opt/${PKG_NAME}"

rm -rf ../dist
mkdir -p ${PREFIX}/bin
mkdir -p ${PREFIX}/etc/sauron
mkdir -p ${PREFIX}/etc/frodo
mkdir -p ${PREFIX}/etc/dummy
mkdir -p ${PREFIX}/etc/bilbo
mkdir -p ${PREFIX}/modules
mkdir -p ${PREFIX}/run
mkdir -p ${PREFIX}/log
mkdir -p ${PREFIX}/init.d
mkdir -p ${PREFIX}/lib

cp ../bin/exposed ${PREFIX}/bin
cp ../bin/archive-sauron.sh ${PREFIX}/bin
cp ../bin/archive-frodo.sh ${PREFIX}/bin
cp ../modules/mod_ccd_sauron.so ${PREFIX}/modules
cp ../modules/mod_ccd_frodo.so ${PREFIX}/modules
cp ../modules/mod_ccd_bilbo.so ${PREFIX}/modules
cp ../etc/exposed-sauron.cfg ${PREFIX}/etc
cp ../etc/exposed-frodo.cfg ${PREFIX}/etc
cp ../etc/exposed-bilbo.cfg ${PREFIX}/etc
cp ../etc/log4crc.sauron ${PREFIX}/etc/sauron/log4crc
cp ../etc/log4crc.frodo ${PREFIX}/etc/frodo/log4crc
cp ../etc/log4crc.bilbo ${PREFIX}/etc/bilbo/log4crc
cp ../init.d/exposed ${PREFIX}/init.d/exposed-sauron
cp ../init.d/exposed ${PREFIX}/init.d/exposed-frodo
cp ../init.d/exposed ${PREFIX}/init.d/exposed-bilbo
cp ../lib/frodo/ARC_API/lib/*.so ${PREFIX}/lib

tar jcf ../dist/${PKG_NAME}.tar.bz2 -C $PREFIX/.. $PKG_NAME
