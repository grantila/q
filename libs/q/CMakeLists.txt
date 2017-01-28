
find_source_tree( LIBQ_HEADERS          "Header Files"          include/q "*.hpp" )
find_source_tree( LIBQ_INTERNAL_HEADERS "Internal Header Files" src       "*.hpp" )
find_source_tree( LIBQ_SOURCES          "Source Files"          src       "*.cpp" )

add_library( q ${LIBQ_HEADERS} ${LIBQ_INTERNAL_HEADERS} ${LIBQ_SOURCES} )

target_link_libraries( q ${CXXLIB} ${GENERIC_LIB_DEPS} )

install( DIRECTORY include/q/ DESTINATION include/q )
install( TARGETS q
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX}
)

