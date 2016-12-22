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

typedef std::shared_ptr< server_socket::pimpl > inner_ref_type;

namespace {

void closer( ::uv_handle_t* handle )
{
	auto socket = reinterpret_cast< ::uv_tcp_t* >( handle );
	auto ref = reinterpret_cast< inner_ref_type* >( socket->data );

	if ( ref )
		delete ref;
};

void server_socket_close( const inner_ref_type& ref )
{
	if ( ref->uv_loop_ )
	{
		::uv_close(
			reinterpret_cast< ::uv_handle_t* >( &ref->socket_ ),
			closer
		);
	}
	ref->uv_loop_ = nullptr;
}

} // anonymous namespace

server_socket::server_socket( std::uint16_t port, ip_addresses&& bind_to )
: pimpl_( q::make_unique< pimpl >( ) )
{
	set_pimpl( &pimpl_->event_ );

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
	auto ref = reinterpret_cast< inner_ref_type* >( pimpl_->socket_.data );
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
	server_socket_close( pimpl_ );
}

void server_socket::sub_attach( const dispatcher_ptr& dispatcher ) noexcept
{
	pimpl_->dispatcher_ = dispatcher;

	auto& dispatcher_pimpl = *pimpl_->dispatcher_->pimpl_;

	const int channel_backlog = 32; // TODO: Reconsider backlog
	const int socket_backlog = 64;  // TODO: Reconsider backlog

	pimpl_->channel_ = std::make_shared< q::channel< socket_ptr > >(
		dispatcher_pimpl.user_queue, channel_backlog );

	auto iter = pimpl_->bind_to_.begin( pimpl_->port_ );

	if ( iter == pimpl_->bind_to_.end( pimpl_->port_ ) )
		Q_THROW( invalid_ip_address( ) );

	auto addr = *iter;

	pimpl_->uv_loop_ = &dispatcher_pimpl.uv_loop;

	::uv_tcp_init( pimpl_->uv_loop_, &pimpl_->socket_ );

	::uv_tcp_bind( &pimpl_->socket_, &*addr, 0 );

	auto ref = new inner_ref_type( pimpl_ );

	pimpl_->socket_.data = reinterpret_cast< void* >( ref );

	uv_connection_cb connection_callback = [ ]( ::uv_stream_t* server, int status )
	{
		auto ref = *reinterpret_cast< inner_ref_type* >( server->data );

		if ( status < 0 )
		{
			// TODO: Consider whether to potentially close the
			//       entire listening server and send an exception
			//       over the channel.
			fprintf(
				stderr,
				"New connection error %s\n",
				::uv_strerror( status ) );
			return;
		}

		if ( !ref )
		{
			// We have just removed this server listener when an
			// incoming socket came in. Just ignore it.
			// This should probably never happen.
			return;
		}

		auto socket_pimpl = std::make_shared< socket::pimpl >( );

		::uv_tcp_init( ref->uv_loop_, &socket_pimpl->socket_ );

		auto client = reinterpret_cast< ::uv_stream_t* >(
			&socket_pimpl->socket_ );

		if ( ::uv_accept( server, client ) == 0 )
		{
			auto client_socket = socket::construct(
				std::move( socket_pimpl ) );

			client_socket->attach( ref->dispatcher_ );

			auto writable = ref->channel_->get_writable( );
			if ( !writable.send( std::move( client_socket ) ) )
				server_socket_close( ref );

			// TODO: Check writable.should_send( ), and stop
			//       listening for new connections until it
			//       notifies to continue.
		}
		else
		{
			auto client_handle =
				reinterpret_cast< ::uv_handle_t* >( client );
			::uv_close( client_handle, nullptr );
		}
	};

	::uv_stream_t* server = reinterpret_cast< ::uv_stream_t* >(
		&pimpl_->socket_ );

	int ret = ::uv_listen( server, socket_backlog, connection_callback );

	if ( ret )
	{
		// TODO: Convert libuv error to exception
		// TODO: Also do uv_strerror( r ) to get message
		throw_by_errno( uv_error_to_errno( ret ) );
	}

}
/*
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

	auto self_ptr = new server_socket_arg_type( self );

	event_callback_fn fn_read = [ ](
		evutil_socket_t fd, short events, void* arg
	)
	-> void
	{
		auto self_ptr = reinterpret_cast< server_socket_arg_type* >(
			arg );

		auto self = self_ptr->server_socket_.lock( );

		if ( events & LIBQ_EV_CLOSE )
		{
			delete self_ptr;
			if ( !!self )
				self->close_channel( );
			return;
		}

		if ( !!self )
			self->on_event_read( );
	};

	pimpl_->ev_ = ::event_new(
		event_base, pimpl_->socket_, EV_READ, fn_read, self_ptr );

	::event_add( pimpl_->ev_, nullptr );

	::event_active( self->pimpl_->ev_, EV_READ, 0 );
}
*/

/*
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
*/

/*
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
*/

/*
void server_socket::close_channel( )
{
	pimpl_->self_.reset( );
}
*/

} } // namespace io, namespace q
