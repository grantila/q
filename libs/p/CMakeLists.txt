
find_source_tree( LIBP_HEADERS          "Header Files"          include/p "*.hpp" )
find_source_tree( LIBP_INTERNAL_HEADERS "Internal Header Files" src       "*.hpp" )
find_source_tree( LIBP_SOURCES          "Source Files"          src       "*.cpp" )

add_library( p ${LIBP_HEADERS} ${LIBP_INTERNAL_HEADERS} ${LIBP_SOURCES} )

target_link_libraries( p q ${CXXLIB} ${GENERIC_LIB_DEPS} )

include_directories( "include" )
