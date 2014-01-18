
set( LIBQ_SOURCES
	main.cpp
)

set( LIBQ_HEADERS )

add_executable( benchmark ${LIBQ_SOURCES} )
target_link_libraries( benchmark q ${CXXLIB} )

