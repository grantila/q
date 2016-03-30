
set( PLAYGROUND_QURL_SOURCES
	main.cpp
)

add_executable( playground-qurl ${PLAYGROUND_QURL_SOURCES} )
message( "EXTRA ${CURL_EXTRA_LIBS}" )
target_link_libraries( playground-qurl qurl q q-io ${LIBCURL_LIB} ${LIBEVENT_LIB} ${CXXLIB} ${CURL_EXTRA_LIBS} )

