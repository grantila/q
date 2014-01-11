
mkdir obj
cd obj

if [ "a$BUILDTYPE" == "a" ]; then
	if [ `uname -s` == Linux ]; then
		BUILDTYPE=Unix Makefiles
	elif [ `uname -s` == Darwin ]; then
		BUILDTYPE=Xcode
	fi
fi

cmake -G "$BUILDTYPE" ..

