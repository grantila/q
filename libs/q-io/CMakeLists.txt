
find_source_tree( LIBQ_HEADERS          "Header Files"          include/q-io "*.hpp" )
find_source_tree( LIBQ_INTERNAL_HEADERS "Internal Header Files" src          "*.hpp" )
find_source_tree( LIBQ_SOURCES          "Source Files"          src          "*.cpp" )

add_library( q-io ${LIBQ_HEADERS} ${LIBQ_INTERNAL_HEADERS} ${LIBQ_SOURCES} )

target_link_libraries( q-io q ${CXXLIB} pthread dl ${LIBUV_LIBRARIES} )

#include_directories( "${Q_ROOT}/3rdparty/libevent-2.1.3-alpha/include/" )
include_directories( SYSTEM ${LIBUV_INCLUDE_DIRS} )

include_directories( "include" )
