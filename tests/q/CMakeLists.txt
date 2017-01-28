
find_source_tree( LIBQ_TEST_HEADERS "Header Files" src "*.hpp" )
find_source_tree( LIBQ_TEST_SOURCES "Source Files" src "*.cpp" )

include_directories( "3rdparty/gmock-1.7.0/include" )

add_executable( q-unit-tests ${LIBQ_TEST_HEADERS} ${LIBQ_TEST_SOURCES} )

target_link_libraries( q-unit-tests q-test q gmock_main ${CXXLIB} )

