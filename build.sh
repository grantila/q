
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

if [ "a$BUILDTYPE" == "a" ]; then
	if [ `uname -s` == Linux ]; then
		BUILDTYPE="Unix Makefiles"
	elif [ `uname -s` == Darwin ]; then
		BUILDTYPE=Xcode
	elif [ `uname -o` == Msys ]; then
		BUILDTYPE=VS
	fi
fi

if [ "a$1" == "adeps" ]; then
	BASEDIR=`pwd`
	THIRD_DIR=${BASEDIR}/3rdparty/dist
	if [ ! -d ${THIRD_DIR}/lib ] ; then
		cd 3rdparty
		./build-deps.sh $BUILDTYPE $CORES
		cd ..
	fi
fi

if [ "$BUILDTYPE" == "Unix Makefiles" ]; then
	mkdir -p obj
	cd obj

	# Create 4 different makefiles (each in a separate directory)
	mkdir release;        cd release ;        cmake -G "$BUILDTYPE" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON  ../.. ; cd ..
	mkdir debug;          cd debug ;          cmake -G "$BUILDTYPE" -DCMAKE_BUILD_TYPE=Debug   -DBUILD_SHARED_LIBS=ON  ../.. ; cd ..
	mkdir release-static; cd release-static ; cmake -G "$BUILDTYPE" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF ../.. ; cd ..
	mkdir debug-static;   cd debug-static ;   cmake -G "$BUILDTYPE" -DCMAKE_BUILD_TYPE=Debug   -DBUILD_SHARED_LIBS=OFF ../.. ; cd ..

	cd release        ; make -j$CORES ; cd ..
	cd debug          ; make -j$CORES ; cd ..
	cd release-static ; make -j$CORES ; cd ..
	cd debug-static   ; make -j$CORES ; cd ..
elif [ "$BUILDTYPE" == "Xcode" ]; then
	cmake -G "$BUILDTYPE" -Bbuild -H.
elif [ "$BUILDTYPE" == "VS" ]; then
	cmake -G "Visual Studio 14 2015" -Bbuild -H.
fi
