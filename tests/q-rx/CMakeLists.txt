
find_source_tree( LIBQRX_TEST_HEADERS "Header Files" src "*.hpp" )
find_source_tree( LIBQRX_TEST_SOURCES "Source Files" src "*.cpp" )

add_executable( q-rx-unit-tests ${LIBQRX_TEST_HEADERS} ${LIBQRX_TEST_SOURCES} )

target_link_libraries( q-rx-unit-tests q q-test q-rx-test gmock_main ${CXXLIB} ${GENERIC_LIB_DEPS} )

