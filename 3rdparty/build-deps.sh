#!/bin/sh

BUILDTYPE=$1
CORES=$2

mkdir -p dist
INSTALLDIR=`pwd`/dist

find_one_dir() {
	\ls -1ld ${1}* 2> /dev/null | grep ^d | wc -l
}

# libuv
if [ $( find_one_dir libuv- ) -eq 1 ]; then
	echo Building libuv
	cd libuv-*
	sh autogen.sh
	./configure --prefix=${INSTALLDIR}
	make -j${CORES}
	make check
	make install
	cd ..
else
	echo Did not find exactly one directory matching 'libuv-*', not building libuv
fi

exit 0

# libcurl
cd curl-*
if [ "$BUILDTYPE" == "Xcode" ]; then
	./configure --prefix=${INSTALLDIR} --with-darwinssl
else
	./configure --prefix=${INSTALLDIR} --without-ssl
fi
make -j${CORES}
make install
