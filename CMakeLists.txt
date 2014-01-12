
cmake_minimum_required( VERSION 2.8 )

project( q )

include( "cmake/projects.cmake" )

add_definitions( "-g" )
add_definitions( "-Wall" )
add_definitions( "-std=c++11" )
if ( NOT CMAKE_COMPILER_IS_GNUCXX )
	add_definitions( "-stdlib=libc++" )
	set( CXXLIB "c++" )
else ( )
	set( CXXLIB "stdc++" )
endif ( )
add_definitions( "-Wno-comment" )

include_directories( "libs/q/include" )

add_subdirectory( "libs/q" )

add_subdirectory( "progs/playground" )

include_directories( "3rdparty/gmock-1.7.0/gtest/include/" )
include_directories( "3rdparty/gmock-1.7.0/include/" )
add_subdirectory( "3rdparty/gmock-1.7.0" )

add_subdirectory( "tests/q" )

