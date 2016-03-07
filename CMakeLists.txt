
cmake_minimum_required( VERSION 2.8 )

project( q )

set( Q_ROOT ${PROJECT_SOURCE_DIR} )

include( "cmake/projects.cmake" )

set( CMAKE_DEBUG_POSTFIX "d" )

if ( MSVC )
	set( STATIC_LIB_SUFFIX 'lib' )
	set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd" )
	set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT" )
else ( )
	set( STATIC_LIB_SUFFIX 'a' )
	set( CMAKE_CXX_FLAGS "-g" )
	set( CMAKE_CXX_FLAGS_RELEASE "-O2" )

	add_definitions( "-Wall" )
	add_definitions( "-std=c++11" )

	if ( NOT CMAKE_COMPILER_IS_GNUCXX )
		add_definitions( "-stdlib=libc++" )
		set( CXXLIB "c++" )
	else ( )
		set( CXXLIB "stdc++" )
	endif ( )

	add_definitions( "-Wno-comment" )

        set( GENERIC_LIB_DEPS pthread dl )
endif ( )

include_directories( "libs/q/include" )

include_directories( "3rdparty/dist/include" )
set( LIBEVENT_LIB "${Q_ROOT}/3rdparty/dist/lib/libevent.${STATIC_LIB_SUFFIX}" )

add_subdirectory( "libs/q" )

include_directories( "3rdparty/gmock-1.7.0/gtest/include/" )
include_directories( "3rdparty/gmock-1.7.0/include/" )
include_directories( "libs/q-test/include" )
add_subdirectory( "libs/q-test" )

add_subdirectory( "libs/p" )

include_directories( "3rdparty/dist/include/" )

include_directories( "libs/qio/include" )
add_subdirectory( "libs/qio" )

add_subdirectory( "progs/playground" )
add_subdirectory( "progs/playground-io" )
add_subdirectory( "progs/benchmark" )

add_subdirectory( "3rdparty/gmock-1.7.0" )

add_subdirectory( "tests/q" )

