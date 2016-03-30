
find_source_tree( LIBQURL_HEADERS          "Header Files"          include/qurl "*.hpp" )
find_source_tree( LIBQURL_INTERNAL_HEADERS "Internal Header Files" src          "*.hpp" )
find_source_tree( LIBQURL_SOURCES          "Source Files"          src          "*.cpp" )

add_library( qurl ${LIBQURL_HEADERS} ${LIBQURL_INTERNAL_HEADERS} ${LIBQURL_SOURCES} )

target_link_libraries( qurl q-io q ${CXXLIB} pthread dl ${LIBEVENT_LIB} )

