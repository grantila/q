#!/bin/sh
mkdir -p dist
INSTALLDIR=`pwd`/dist
cd libevent-2.1.5*
./configure --prefix=${INSTALLDIR} --disable-openssl
make
make install
