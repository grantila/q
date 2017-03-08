find_source_tree( LIBQ_HEADERS          "Header Files"          include/q "*.hpp" )
find_source_tree( LIBQ_INTERNAL_HEADERS "Internal Header Files" src       "*.hpp" )
find_source_tree( LIBQ_SOURCES          "Source Files"          src       "*.cpp" )

add_library( q ${LIBQ_SOURCES} )
target_include_directories (q PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE ${LIBQ_INTERNAL_HEADERS})

target_link_libraries( q ${CXXLIB} ${GENERIC_LIB_DEPS} )

install( DIRECTORY include/q/ DESTINATION include/q )
install( TARGETS q
        EXPORT QConfig
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX}
)
install(EXPORT QConfig DESTINATION share/q/cmake)
export(TARGETS q FILE QConfig.cmake)
