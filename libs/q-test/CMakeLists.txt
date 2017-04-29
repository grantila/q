
find_source_tree( LIBQTEST_HEADERS "Header Files" include/q-test "*.hpp" )

# For CMake 3 (and above) we export an interface-only library, but for
# CMake 2.x we use an empty dummy translation unit to create a linkable
# library.

# Update; CMake's INTERFACE libraries are pretty broken, so we go with the a
# static dummy library for now.

# if ( ${CMAKE_MAJOR_VERSION} GREATER 2 )
if ( FALSE )
	set( QTEST_HEADER_ONLY TRUE )
else ( )
	set( QTEST_HEADER_ONLY FALSE )
endif ( )

if ( ${QTEST_HEADER_ONLY} )
	add_library( q-test INTERFACE )

	# This doesn't work with CMake yet, so IDE's won't create the project
	# which is why this is disabled for now
	target_sources( q-test INTERFACE ${LIBQTEST_HEADERS} )

	target_include_directories( q-test
		INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
	)
else ( )
	add_library( q-test STATIC ${LIBQTEST_HEADERS} src/dummy.cpp )

	target_include_directories( q-test
		PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
	)

	target_link_libraries( q-test q ${CXXLIB} ${GENERIC_LIB_DEPS} )
endif ( )

install( DIRECTORY include/q-test/ DESTINATION include/q-test )
install( TARGETS q-test
	EXPORT QTestConfig
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX}
)

install( EXPORT QTestConfig DESTINATION share/q-test/cmake )
export( TARGETS q-test q FILE QTestConfig.cmake )
