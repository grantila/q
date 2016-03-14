
set( LIBQ_SOURCES
	main.cpp
)

set( LIBQ_HEADERS )

add_executable( playground-io ${LIBQ_SOURCES} )
target_link_libraries( playground-io q q-io ${LIBEVENT_LIB} ${CXXLIB} )

