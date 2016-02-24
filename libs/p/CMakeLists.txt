
find_source_tree( LIBQ_HEADERS          "Header Files"          include/p "*.hpp" )
find_source_tree( LIBQ_INTERNAL_HEADERS "Internal Header Files" src       "*.hpp" )
find_source_tree( LIBQ_SOURCES          "Source Files"          src       "*.cpp" )

add_library( p SHARED ${LIBQ_HEADERS} ${LIBQ_INTERNAL_HEADERS} ${LIBQ_SOURCES} )

target_link_libraries( p q ${CXXLIB} ${GENERIC_LIB_DEPS} )

include_directories( "include" )
