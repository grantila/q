
find_source_tree( LIBQRX_TEST_HEADERS "Header Files" src "*.hpp" )
find_source_tree( LIBQRX_TEST_SOURCES "Source Files" src "*.cpp" )

add_executable( q-rx-test ${LIBQRX_TEST_HEADERS} ${LIBQRX_TEST_SOURCES} )

target_link_libraries( q-rx-test q gmock_main ${CXXLIB} ${GENERIC_LIB_DEPS} )

