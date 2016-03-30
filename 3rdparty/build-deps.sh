#!/bin/sh

BUILDTYPE=$1
CORES=$2

mkdir -p dist
INSTALLDIR=`pwd`/dist

# libevent
cd libevent-2.1.5*
./configure --prefix=${INSTALLDIR} --disable-openssl
make
make install

# libcurl
cd curl-*
if [ "$BUILDTYPE" == "Xcode" ]; then
	./configure --prefix=${INSTALLDIR} --with-darwinssl
else
	./configure --prefix=${INSTALLDIR} --without-ssl
fi
make -j${CORES}
make install
