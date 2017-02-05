cmake_minimum_required( VERSION 2.8 )

project( q )

# recommandation, do not build in source.
set( CMAKE_DISABLE_IN_SOURCE_BUILD ON )

# required for library API/ABI tagging
set( q_MAJOR_VERSION 0 )
set( q_MINOR_VERSION 0 )
set( q_PATCH_VERSION 1 )
set( q_VERSION ${q_MAJOR_VERSION}.${q_MINOR_VERSION}.${q_PATCH_VERSION} )

# if no build config given, we decide
if ( NOT CMAKE_BUILD_TYPE )
	set(
		CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
		FORCE )
endif ( )


include( "cmake/projects.cmake" )

include( "cmake/colors.cmake" )

# turn on verbose compilation output
if ( NOT DEFINED ${PROJECT_NAME}_VERBOSE_COMPILE )
	option( ${PROJECT_NAME}_VERBOSE_COMPILE "verbose compile" OFF )
endif ( )

# set up the comiler
include( "cmake/compilersetup.cmake" )

# show me what to install
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

# make testing optional
if ( DEFINED ${PROJECT_NAME}_BUILD_TESTS )
	option(
		${PROJECT_NAME}_BUILD_TESTS
		"build tests"
		${PROJECT_NAME}_BUILD_TESTS )
else ( )
	option( ${PROJECT_NAME}_BUILD_TESTS "build tests" ON )
endif ( )


include_directories( "libs/q/include" )
add_subdirectory( "libs/q" )

if ( ${PROJECT_NAME}_BUILD_TESTS )
	enable_testing()
	include_directories( "3rdparty/gmock-1.7.0/gtest/include/" )
	include_directories( "3rdparty/gmock-1.7.0/include/" )
	include_directories( "libs/q-test/include" )
	add_subdirectory( "libs/q-test" )
	add_subdirectory( "3rdparty/gmock-1.7.0" )
	add_subdirectory( "tests" )
endif ( )


# TODO does not exits, can be removed ?
# include_directories( "3rdparty/dist/include/" )

# make apps optional
if ( DEFINED ${PROJECT_NAME}_BUILD_APPS )
	option(
		${PROJECT_NAME}_BUILD_APPS
		"build sample and playgournd apps"
		${PROJECT_NAME}_BUILD_APPS )
else ( )
	option( ${PROJECT_NAME}_BUILD_APPS "build tests" ON )
endif ( )

if ( ${PROJECT_NAME}_BUILD_APPS )
	add_subdirectory( "progs/playground" )
	add_subdirectory( "progs/benchmark" )
endif ( )

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/libq.pc.cmake.cfg"
	"${CMAKE_BINARY_DIR}/libq.pc"
	@ONLY )
install(
	FILES "${CMAKE_BINARY_DIR}/libq.pc"
	DESTINATION lib${LIB_SUFFIX}/pkgconfig )

