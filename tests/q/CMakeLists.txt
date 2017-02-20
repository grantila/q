
find_source_tree( LIBQ_TEST_HEADERS "Header Files" src "*.hpp" )
find_source_tree( LIBQ_TEST_SOURCES "Source Files" src "*.cpp" )


add_executable( q-unit-tests ${LIBQ_TEST_HEADERS} ${LIBQ_TEST_SOURCES} )

target_link_libraries( q-unit-tests q-test q ${LIBQ_GTEST_LIB} ${CXXLIB} ${GENERIC_LIB_DEPS})

add_test( NAME q-unit-tests COMMAND  q-unit-tests )
