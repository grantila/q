
find_source_tree( LIBQRX_HEADERS          "Header Files"          include/q-rx "*.hpp" )
find_source_tree( LIBQRX_INTERNAL_HEADERS "Internal Header Files" src       "*.hpp" )
find_source_tree( LIBQRX_SOURCES          "Source Files"          src       "*.cpp" )

add_library( q-rx SHARED ${LIBQRX_HEADERS} ${LIBQRX_INTERNAL_HEADERS} ${LIBQRX_SOURCES} )

target_link_libraries( q-rx q ${CXXLIB} ${GENERIC_LIB_DEPS} )

include_directories( "include" )
