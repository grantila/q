#!/bin/sh

BUILDTYPE=$1
CORES=$2

mkdir -p dist
INSTALLDIR=`pwd`/dist

# libevent
cd libevent-2.1.5*
./configure --prefix=${INSTALLDIR} --disable-openssl
make -j${CORES}
make install
cd ..

# libuv
cd libuv-v1.*
sh autogen.sh
./configure --prefix=${INSTALLDIR}
make -j${CORES}
make check
make install
cd ..

# libcurl
cd curl-*
if [ "$BUILDTYPE" == "Xcode" ]; then
	./configure --prefix=${INSTALLDIR} --with-darwinssl
else
	./configure --prefix=${INSTALLDIR} --without-ssl
fi
make -j${CORES}
make install
