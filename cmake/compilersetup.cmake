
if ( NOT CMAKE_CXX_STANDARD )
	set( CMAKE_CXX_STANDARD 11 )
endif ( )

set( CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD} CACHE STRING
	"C++ standard to use, currently 11 or 14."
	FORCE )

set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

# for all possible values see
# https://cmake.org/cmake/help/v3.0/variable/CMAKE_LANG_COMPILER_ID.html
if ( CMAKE_CXX_COMPILER_ID MATCHES "GNU" )

	if ( CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9 )
		set( CMAKE_CXX_FLAGS "-std=c++11 -Wall -pipe -pthread" )
	endif ( )

	if ( NOT CMAKE_CXX_FLAGS )
		set( CMAKE_CXX_FLAGS "-Wall -Wextra -pipe -pthread" )

		set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE STRING
			"Flags used by the compiler during all build types."
			FORCE )
	endif ( )

	if ( NOT CMAKE_EXE_LINKER_FLAGS )
		set( CMAKE_EXE_LINKER_FLAGS "-ldl -lpthread" )
		set( CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} CACHE STRING
			"Flags used by the linker during the creation of modules."
			FORCE )
	endif ( )

	# some need them always in at the end, so add them always
	set( GENERIC_LIB_DEPS dl pthread )

	if ( ${PROJECT_NAME}_VERBOSE_COMPILE )
	        add_definitions( "-ftemplate-backtrace-limit=0" )
	endif ( )

elseif ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )

	if ( NOT CMAKE_CXX_FLAGS )
		# use clang std, this enables using of llvm on old platforms, like RHEL6
		set( CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++ -Wall -pipe -pthread" )

		set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE STRING
			"Flags used by the compiler during all build types."
			FORCE )
		endif ( )

	if ( NOT CMAKE_EXE_LINKER_FLAGS )
		set( CMAKE_EXE_LINKER_FLAGS "-lc++ -lc++abi -ldl -lpthread" )
		set( CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} CACHE STRING
			"Flags used by the linker during the creation of modules."
			FORCE )
	endif ( )

	if ( ${PROJECT_NAME}_VERBOSE_COMPILE )
	        add_definitions( "-ftemplate-backtrace-limit=0" )
	endif ( )

elseif ( CMAKE_CXX_COMPILER_ID MATCHES "AppleClang" )

	if ( NOT CMAKE_CXX_FLAGS )
		# recommand this
		set( CMAKE_CXX_FLAGS "-Wall -pipe -pthread" )

		set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE STRING
			"Flags used by the compiler during all build types."
			FORCE )
	endif ( )

	if ( ${PROJECT_NAME}_VERBOSE_COMPILE )
	        add_definitions( "-ftemplate-backtrace-limit=0" )
	endif ( )

elseif ( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )

	set( CMAKE_DEBUG_POSTFIX "d" )
	add_definitions( "/bigobj" )

	set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd" )
	set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT" )

else ( )

	q_warning( "untested compiler: ${CMAKE_CXX_COMPILER}" )
	q_message( "Please setup all required flags yourself." )

endif ( )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )



