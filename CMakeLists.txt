
cmake_minimum_required( VERSION 3.1 )

project( q )

include( "cmake/projects.cmake" )

set( CMAKE_DEBUG_POSTFIX "d" )

# Require 11, fallback to 11 too
if ( NOT CMAKE_CXX_STANDARD )
	message( "No C++ standard provided, falling back to C++11" )
	set( CMAKE_CXX_STANDARD 11 )
else ( )
	if ( CMAKE_CXX_STANDARD LESS 11 )
		message( FATAL_ERROR
			"Too old C++ standard (${CMAKE_CXX_STANDARD}), q requires C++11" )
	endif( )

	message( "Using user-provided C++ standard ${CMAKE_CXX_STANDARD}" )
endif ( )
set ( CXX_STANDARD_REQUIRED ON )

if ( MSVC )
	set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd" )
	set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT" )
else ( )
	set( CMAKE_CXX_FLAGS "-g" )
	set( CMAKE_CXX_FLAGS_RELEASE "-O2" )

	add_definitions( "-Wall" )
	add_definitions( "-Wno-comment" )

#        set( GENERIC_LIB_DEPS pthread dl )
endif ( )

include_directories( "libs/q/include" )
add_subdirectory( "libs/q" )

include_directories( "3rdparty/gmock-1.7.0/gtest/include/" )
include_directories( "3rdparty/gmock-1.7.0/include/" )
include_directories( "libs/q-test/include" )
add_subdirectory( "libs/q-test" )

#add_subdirectory( "libs/p" )

include_directories( "3rdparty/dist/include/" )

add_subdirectory( "progs/playground" )
add_subdirectory( "progs/benchmark" )

add_subdirectory( "3rdparty/gmock-1.7.0" )

add_subdirectory( "tests/qtest" )
add_subdirectory( "tests/q" )

if ( NOT WIN32 )
	string( ASCII 27 Esc )
	set( ColorReset "${Esc}[m" )
	set( Green      "${Esc}[32m" )
	set( Magenta    "${Esc}[35m" )
	set( Cyan       "${Esc}[36m" )
endif ( )

message( "${Green}q project successfully created!${ColorReset}" )
if ( APPLE )
	message( "${Green}You can now open "
		"${Cyan}${CMAKE_BINARY_DIR}/q.xcodeproj${ColorReset}" )
elseif ( UNIX )
	message( "${Green}You can now enter "
		"${Cyan}${CMAKE_BINARY_DIR}/${Green} and make the libraries"
		"${ColorReset}" )
elseif ( MSVC )
	message( "${Green}You can now open "
		"${Cyan}${CMAKE_BINARY_DIR}/q.sln${ColorReset}" )
endif ( )
