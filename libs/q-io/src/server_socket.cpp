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
	std::shared_ptr< q::channel< socket_ptr > > channel_;
	socket_t socket_;

	dispatcher_ptr dispatcher_;

	std::size_t backlog_;

	::event* ev_;

	std::atomic< bool > can_read_;

	server_socket_ptr self_;

	std::atomic< bool > closed_;
};

server_socket::server_socket( socket_t sock )
: pimpl_( q::make_unique< pimpl >( ) )
{
	pimpl_->channel_ = nullptr;
	pimpl_->socket_ = sock;
	pimpl_->dispatcher_ = nullptr;
	pimpl_->backlog_ = 128; // TODO: Reconsider backlog
	pimpl_->ev_ = nullptr;
	pimpl_->can_read_ = true;
	pimpl_->self_ = nullptr;
	pimpl_->closed_ = false;
}

server_socket::~server_socket( )
{
	if ( pimpl_->ev_ )
		::event_free( pimpl_->ev_ );

	::evutil_closesocket( pimpl_->socket_ );
}

server_socket_ptr server_socket::construct( socket_t sock )
{
	return q::make_shared< server_socket >( sock );
}

q::readable< socket_ptr > server_socket::clients( )
{
	return pimpl_->channel_->get_readable( );
}

void server_socket::sub_attach( const dispatcher_ptr& dispatcher ) noexcept
{
	auto& dispatcher_pimpl = get_dispatcher_pimpl( );
	auto event_base = dispatcher_pimpl.event_base;

	pimpl_->dispatcher_ = dispatcher;

	pimpl_->channel_ = std::make_shared< q::channel< socket_ptr > >(
		dispatcher_pimpl.user_queue, pimpl_->backlog_ );

	auto self = shared_from_this( );

	pimpl_->channel_->get_writable( ).set_resume_notification( [ self ]( )
	{
		if ( self->pimpl_->closed_ )
			return;
		::event_active( self->pimpl_->ev_, EV_READ, 0 );
	} );

	auto fn_read = [ ]( evutil_socket_t fd, short events, void* arg )
	-> void
	{
		auto self = reinterpret_cast< server_socket* >( arg );

		if ( events & LIBQ_EV_CLOSE )
		{
			self->close_channel( );
			return;
		}

		self->on_event_read( );
	};

	pimpl_->ev_ = ::event_new(
		event_base, pimpl_->socket_, EV_READ, fn_read, this );

	::event_add( pimpl_->ev_, nullptr );

	::event_active( self->pimpl_->ev_, EV_READ, 0 );
}

void server_socket::on_event_read( ) noexcept
{
	bool done = false;

	auto writable = pimpl_->channel_->get_writable( );

	while ( writable.should_send( ) )
	{
		struct sockaddr_in peer_addr;
		socklen_t peer_addr_len = sizeof peer_addr;

		auto ret = ::accept(
			pimpl_->socket_,
			reinterpret_cast< sockaddr* >( &peer_addr ),
			&peer_addr_len );

		if ( ret != -1 )
		{
			// Got connection
			prepare_socket( ret );
			auto socket_ptr = socket::construct( ret );

			socket_ptr->attach( get_dispatcher( ) );

			writable.send( socket_ptr );
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
				writable.close( );
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

	pimpl_->channel_->get_writable( ).set_resume_notification( nullptr );

	EVUTIL_CLOSESOCKET( pimpl_->socket_ );

	::event_del( pimpl_->ev_ );

	::event_active( pimpl_->ev_, LIBQ_EV_CLOSE, 0 );

	pimpl_->channel_->get_writable( ).close( );
}

void server_socket::close_channel( )
{
	pimpl_->self_.reset( );
}

} } // namespace io, namespace q
