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

#include "internals.hpp"
#include "socket_helpers.hpp"

namespace q { namespace io {

server_socket::server_socket( std::uint16_t port, ip_addresses&& bind_to )
: pimpl_( q::make_unique< pimpl >( ) )
{
//	set_pimpl( &pimpl_->event_ );

	pimpl_->port_ = port;
	pimpl_->bind_to_ = std::move( bind_to );

	pimpl_->socket_.data = nullptr;
	pimpl_->uv_loop_ = nullptr;

/*
	pimpl_->channel_ = nullptr;
	pimpl_->socket_ = sock;
	pimpl_->dispatcher_ = nullptr;
	pimpl_->backlog_ = 128; // TODO: Reconsider backlog
	pimpl_->ev_ = nullptr;
	pimpl_->can_read_ = true;
	pimpl_->self_ = nullptr;
	pimpl_->closed_ = false;
*/
}

server_socket::~server_socket( )
{
	close( );
/*
	auto ref = reinterpret_cast< server_socket::pimpl::data_ref_type* >(
		pimpl_->socket_.data );
	if ( ref )
		delete ref;
*/

/*
	if ( pimpl_->ev_ )
		::event_free( pimpl_->ev_ );

	::evutil_closesocket( pimpl_->socket_ );
*/
}

server_socket_ptr
server_socket::construct( std::uint16_t port, ip_addresses&& bind_to )
{
	return q::make_shared< server_socket >( port, std::move( bind_to ) );
}

q::readable< socket_ptr > server_socket::clients( )
{
	return pimpl_->channel_->get_readable( );
}

void server_socket::close( )
{
	// if detached, just let go
	pimpl_->close( );
}

} } // namespace io, namespace q
