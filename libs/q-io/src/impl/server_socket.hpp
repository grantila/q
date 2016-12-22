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

#ifndef LIBQIO_INTERNAL_IMPL_SERVER_SOCKET_HPP
#define LIBQIO_INTERNAL_IMPL_SERVER_SOCKET_HPP

#include <q-io/server_socket.hpp>

#include "stream.hpp"

namespace q { namespace io {

struct server_socket::pimpl
: stream
, std::enable_shared_from_this< server_socket::pimpl >
{
	typedef std::shared_ptr< server_socket::pimpl > data_ref_type;

//	event::pimpl event_;

	std::shared_ptr< q::channel< socket_ptr > > channel_;

	std::uint16_t port_;
	ip_addresses bind_to_;

	dispatcher_ptr dispatcher_;

	::uv_loop_t* uv_loop_;

	::uv_tcp_t socket_;

	void close( ) override;

	pimpl( )
	: stream( reinterpret_cast< ::uv_stream_t* >( &socket_ ) )
	{
		std::cout << "CONSTRUCTING socket_server::pimpl" << std::endl;
	}
	~pimpl( )
	{
		std::cout << "DESTRUCTING socket_server::pimpl" << std::endl;
	}

	void
	attach_dispatcher( const dispatcher_ptr& dispatcher ) noexcept override;
};

} } // namespace io, namespace q

#endif // LIBQIO_INTERNAL_IMPL_SERVER_SOCKET_HPP
