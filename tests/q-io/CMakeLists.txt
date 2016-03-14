
find_source_tree( LIBQ_IO_TEST_HEADERS "Header Files" src "*.hpp" )
find_source_tree( LIBQ_IO_TEST_SOURCES "Source Files" src "*.cpp" )

add_executable( q-io-unit-tests ${LIBQ_IO_TEST_HEADERS} ${LIBQ_IO_TEST_SOURCES} )

target_link_libraries( q-io-unit-tests q q-io q-test gmock_main ${CXXLIB} ${GENERIC_LIB_DEPS} )

