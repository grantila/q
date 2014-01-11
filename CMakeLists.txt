
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
add_subdirectory( "libs/p" )

add_subdirectory( "progs/playground" )

