
find_source_tree( LIBQTEST_HEADERS          "Header Files"          include/q-test "*.hpp" )
find_source_tree( LIBQTEST_INTERNAL_HEADERS "Internal Header Files" src            "*.hpp" )
find_source_tree( LIBQTEST_SOURCES          "Source Files"          src            "*.cpp" )

add_library( q-test ${LIBQTEST_HEADERS} ${LIBQTEST_INTERNAL_HEADERS} ${LIBQTEST_SOURCES} )

target_link_libraries( q-test q ${CXXLIB} ${GENERIC_LIB_DEPS} )

