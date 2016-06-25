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

#ifndef LIBQIO_SERVER_SOCKET_HPP
#define LIBQIO_SERVER_SOCKET_HPP

#include <q-io/event.hpp>
#include <q-io/ip.hpp>
#include <q-io/socket.hpp>
#include <q-io/types.hpp>

namespace q { namespace io {

/**
 * A server_socket is a socket which allows incoming connections.
 */
class server_socket
: public event
, public std::enable_shared_from_this< server_socket >
{
public:
	struct pimpl;

	~server_socket( );

	/**
	 * Channel used to get incoming connections.
	 */
	q::readable< socket_ptr > clients( );

protected:
	static server_socket_ptr
	construct( std::uint16_t port, ip_addresses&& bind_to );

private:
	server_socket( std::uint16_t port, ip_addresses&& bind_to );

	friend class dispatcher;

	template< typename T > friend class q::shared_constructor;

	void sub_attach( const dispatcher_ptr& dispatcher ) noexcept override;

	std::shared_ptr< pimpl > pimpl_;
};

} } // namespace io, namespace q

#endif // LIBQIO_SERVER_SOCKET_HPP
