
set( LIBQ_SOURCES
	main.cpp
)

set( LIBQ_HEADERS )

add_executable( playground ${LIBQ_SOURCES} )
target_link_libraries( playground q ${CXXLIB} )

