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

#include <event2/event.h>

namespace q { namespace io {

struct server_socket::pimpl
{
	q::channel_ptr< socket_ptr > channel_;
	native_socket socket_;

	dispatcher_ptr dispatcher_;

	::event* ev_;

	std::atomic< bool > can_read_;

	server_socket_ptr self_;

	std::atomic< bool > closed_;
};

server_socket::server_socket( const native_socket& sock )
: pimpl_( q::make_unique< pimpl >( ) )
{
	pimpl_->channel_ = nullptr;
	pimpl_->socket_ = sock;
	pimpl_->dispatcher_ = nullptr;
	pimpl_->ev_ = nullptr;
	pimpl_->can_read_ = true;
	pimpl_->self_ = nullptr;
	pimpl_->closed_ = false;
}

server_socket::~server_socket( )
{
	if ( pimpl_->ev_ )
		::event_free( pimpl_->ev_ );

	::evutil_closesocket( pimpl_->socket_.fd );
}

server_socket_ptr server_socket::construct( const native_socket& sock )
{
	return q::make_shared< server_socket >( sock );
}

q::readable_ptr< socket_ptr > server_socket::clients( )
{
	return pimpl_->channel_;
}

void server_socket::sub_attach( const dispatcher_ptr& dispatcher ) noexcept
{
	auto& dispatcher_pimpl = get_dispatcher_pimpl( );
	auto event_base = dispatcher_pimpl.event_base;

	pimpl_->dispatcher_ = dispatcher;

	pimpl_->channel_ = std::make_shared< q::channel< socket_ptr > >(
		dispatcher_pimpl.user_queue, 128 ); // TODO: Reconsider backlog

	auto self = shared_from_this( );

	pimpl_->channel_->set_resume_notification( [ self ]( )
	{
		std::cout << "WILL RETRY" << std::endl;
		::event_active( self->pimpl_->ev_, EV_READ, 0 );
	} );

	auto fn_read = [ ]( evutil_socket_t fd, short events, void* arg )
	-> void
	{
		std::cout << "running socket event callback [read]" << std::endl;
		auto self = reinterpret_cast< server_socket* >( arg );

		if ( events & LIBQ_EV_CLOSE )
		{
			self->close_channel( );
			return;
		}

		self->on_event_read( );
	};

	pimpl_->ev_ = ::event_new(
		event_base, pimpl_->socket_.fd, EV_READ, fn_read, this );

	::event_add( pimpl_->ev_, nullptr );

	::event_active( self->pimpl_->ev_, EV_READ, 0 );
}

void server_socket::on_event_read( ) noexcept
{
	bool done = false;

	while ( pimpl_->channel_->should_send( ) )
	{
		struct sockaddr_in peer_addr;
		socklen_t peer_addr_len = sizeof peer_addr;

		auto ret = ::accept( pimpl_->socket_.fd, reinterpret_cast< sockaddr* >( &peer_addr ), &peer_addr_len );

		if ( ret != -1 )
		{
			// Got connection
			prepare_socket( ret );
			auto socket_ptr = socket::construct( native_socket{ ret } );

			socket_ptr->attach( get_dispatcher( ) );

			pimpl_->channel_->send( socket_ptr );
		}
		else
		{
			auto errno_ = errno;
			if ( errno_ == EAGAIN )
				// Await socket readability
				done = true;
			else
			{
				pimpl_->closed_ = true;

				// TODO: Don't just close, close with
				// translated errno error
				pimpl_->channel_->close( );
			}

			break;
		}
	}

	if ( done )
	{
		// Done all accepts so far, await more incoming sockets
		::event_add( pimpl_->ev_, nullptr );
		pimpl_->can_read_ = false;
	}
}

void server_socket::close_socket( )
{
	auto was_closed = pimpl_->closed_.exchange( true );

	if ( was_closed )
		return;

	EVUTIL_CLOSESOCKET( pimpl_->socket_.fd );

	::event_del( pimpl_->ev_ );

	::event_active( pimpl_->ev_, LIBQ_EV_CLOSE, 0 );

	pimpl_->channel_->close( );
}

void server_socket::close_channel( )
{
	pimpl_->channel_->set_resume_notification( nullptr );

	pimpl_->self_.reset( );
}

} } // namespace io, namespace q
