/*
 * Copyright 2016 Gustaf Räntilä
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <event2/event.h>
#include <event2/dns.h>

#include <q-io/dispatcher.hpp>

namespace q { namespace io {

static inline void prepare_socket( evutil_socket_t socket )
{
	evutil_make_socket_nonblocking( socket );
}

static inline evutil_socket_t create_socket( int family )
{
	evutil_socket_t socket = ::socket( family, SOCK_STREAM, 6 );

	prepare_socket( socket );

	return socket;
}

template< typename Sockaddr >
static inline
typename std::enable_if<
	std::is_same<
		typename std::decay< Sockaddr >::type,
		sockaddr_in
	>::value
	or
	std::is_same<
		typename std::decay< Sockaddr >::type,
		sockaddr_in6
	>::value,
	std::pair< bool, int >
>::type
connect( evutil_socket_t socket, const Sockaddr& addr )
{
	auto ret = ::connect(
		socket,
		reinterpret_cast< const sockaddr* >( &addr ),
		sizeof addr );

	if ( ret != 0 and ( ret != -1 or errno != EINPROGRESS ) )
		return std::make_pair( false, errno );

	return std::make_pair( true, 0 );
}

} } // namespace io, namespace q
