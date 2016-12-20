
cmake_minimum_required( VERSION 3.1 )

project( q )

set( Q_ROOT ${PROJECT_SOURCE_DIR} )

include( "cmake/projects.cmake" )
include( "cmake/colors.cmake" )

set( CMAKE_DEBUG_POSTFIX "d" )

# Require 11, fallback to 11 too
if ( NOT CMAKE_CXX_STANDARD )
	q_info( "No C++ standard provided, falling back to C++11" )
	set( CMAKE_CXX_STANDARD 11 )
else ( )
	if ( CMAKE_CXX_STANDARD LESS 11 )
		q_fatal( FATAL_ERROR
			"Too old C++ standard (${CMAKE_CXX_STANDARD}), q requires C++11" )
	endif( )

	q_message( "Using user-provided C++ standard ${CMAKE_CXX_STANDARD}" )
endif ( )
set ( CXX_STANDARD_REQUIRED ON )

if ( MSVC )
	set( STATIC_LIB_SUFFIX 'lib' )
	set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd" )
	set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT" )
else ( )
	set( STATIC_LIB_SUFFIX 'a' )
	set( CMAKE_CXX_FLAGS "-g" )
	set( CMAKE_CXX_FLAGS_RELEASE "-O2" )

	add_definitions( "-Wall" )
	add_definitions( "-Wno-comment" )

#        set( GENERIC_LIB_DEPS pthread dl )
endif ( )

include_directories( "libs/q/include" )

include_directories( "3rdparty/dist/include" )
set( LIBEVENT_LIB "${Q_ROOT}/3rdparty/dist/lib/libevent.${STATIC_LIB_SUFFIX}" )
set( LIBUV_LIB "${Q_ROOT}/3rdparty/dist/lib/libuv.${STATIC_LIB_SUFFIX}" )

set( LIBCURL_LIB "${Q_ROOT}/3rdparty/dist/lib/libcurl.${STATIC_LIB_SUFFIX}" )
IF (APPLE)
	FIND_LIBRARY( COREFOUNDATION_LIBRARY CoreFoundation )
	FIND_LIBRARY( SECURITY_LIBRARY Security )
	SET( CURL_EXTRA_LIBS ${COREFOUNDATION_LIBRARY} ${SECURITY_LIBRARY} "ldap" "z" )
ENDIF (APPLE)


add_subdirectory( "libs/q" )

include_directories( "3rdparty/gmock-1.7.0/gtest/include/" )
include_directories( "3rdparty/gmock-1.7.0/include/" )
include_directories( "libs/q-test/include" )
add_subdirectory( "libs/q-test" )

#add_subdirectory( "libs/p" )

include_directories( "3rdparty/dist/include/" )

include_directories( "libs/q-io/include" )
add_subdirectory( "libs/q-io" )

include_directories( "libs/qurl/include" )
add_subdirectory( "libs/qurl" )

add_subdirectory( "progs/playground" )
add_subdirectory( "progs/playground-io" )
add_subdirectory( "progs/playground-qurl" )
add_subdirectory( "progs/benchmark" )

add_subdirectory( "3rdparty/gmock-1.7.0" )

add_subdirectory( "tests/qtest" )
add_subdirectory( "tests/q" )
add_subdirectory( "tests/q-io" )

q_message( "q project successfully created!" )
if ( APPLE )
	q_message( "You can now open " PATH "${CMAKE_BINARY_DIR}/q.xcodeproj" )
elseif ( UNIX )
	q_message( "You can now enter " PATH "${Cyan}${CMAKE_BINARY_DIR}/"
		" and make the libraries" )
elseif ( MSVC )
	q_message( "You can now open " PATH "${CMAKE_BINARY_DIR}/q.sln" )
endif ( )
