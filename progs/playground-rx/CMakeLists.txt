
set( PLAYGROUND_RX_SOURCES
	main.cpp
)

set( PLAYGROUND_RX_HEADERS )

add_executable( playground-rx ${PLAYGROUND_RX_SOURCES} )
target_link_libraries( playground-rx q ${CXXLIB} )
