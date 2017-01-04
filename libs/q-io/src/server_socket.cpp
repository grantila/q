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

#include <q-io/server_socket.hpp>

#include "impl/server_socket.hpp"
#include "socket_helpers.hpp"

namespace q { namespace io {

server_socket::server_socket( std::uint16_t port, ip_addresses&& bind_to )
: pimpl_( q::make_unique< pimpl >( ) )
{
	pimpl_->port_ = port;
	pimpl_->bind_to_ = std::move( bind_to );

	pimpl_->socket_.data = nullptr;
	pimpl_->uv_loop_ = nullptr;
}

server_socket::~server_socket( )
{
	// TODO: Implement detach, and if detached, don't close
/*
	if ( !pimpl_->detached_ )
*/
		close( );
}

server_socket_ptr
server_socket::construct( std::uint16_t port, ip_addresses&& bind_to )
{
	return q::make_shared< server_socket >( port, std::move( bind_to ) );
}

q::readable< tcp_socket_ptr > server_socket::clients( )
{
	return pimpl_->channel_->get_readable( );
}

void server_socket::close( )
{
	std::weak_ptr< pimpl > weak_pimpl = pimpl_;

	pimpl_->dispatcher_->internal_queue_->push( [ weak_pimpl ]( )
	{
		auto pimpl = weak_pimpl.lock( );
		if ( pimpl )
			pimpl->i_close( );
	} );
}

} } // namespace io, namespace q
