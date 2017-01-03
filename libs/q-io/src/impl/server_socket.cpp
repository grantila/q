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

#include "server_socket.hpp"

#include "dispatcher.hpp"
#include "tcp_socket.hpp"

namespace q { namespace io {

namespace {

void closer( ::uv_handle_t* handle )
{
	auto socket = reinterpret_cast< ::uv_tcp_t* >( handle );
	auto ref = reinterpret_cast< server_socket::pimpl::data_ref_type* >(
		socket->data );
	socket->data = nullptr;

	if ( ref )
		delete ref;
};

void server_socket_close( server_socket::pimpl& ref )
{
	if ( ref.uv_loop_ )
	{
		::uv_close( ref.uv_handle( ), closer );
	}
	ref.uv_loop_ = nullptr;
}

} // anonymous namespace

void server_socket::pimpl::close( q::expect< void > exp )
{
	server_socket_close( *this );
}

void
server_socket::pimpl::attach_dispatcher( const dispatcher_ptr& dispatcher )
noexcept
{
	dispatcher_ = dispatcher;

	auto& dispatcher_pimpl = *dispatcher_->pimpl_;

	const int channel_backlog = 32; // TODO: Reconsider backlog
	const int socket_backlog = 64;  // TODO: Reconsider backlog

	channel_ = std::make_shared< q::channel< tcp_socket_ptr > >(
		dispatcher_pimpl.user_queue, channel_backlog );

	auto iter = bind_to_.begin( );

	if ( iter == bind_to_.end( ) )
		Q_THROW( invalid_ip_address( ) );

	auto addr = iter->get_sockaddr( port_ );

	uv_loop_ = &dispatcher_pimpl.uv_loop;

	::uv_tcp_init( uv_loop_, &socket_ );

	::uv_tcp_bind( &socket_, &*addr, 0 );

	auto u_ref = q::make_unique< data_ref_type >( shared_from_this( ) );
	auto ref = u_ref.release( );

	socket_.data = reinterpret_cast< void* >( ref );

	uv_connection_cb connection_callback = [ ]( ::uv_stream_t* server, int status )
	{
		auto ref = *reinterpret_cast< data_ref_type* >( server->data );

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

		auto socket_pimpl = q::make_shared< tcp_socket::pimpl >( );

		::uv_tcp_init( ref->uv_loop_, &socket_pimpl->socket_ );

		if ( ::uv_accept( server, socket_pimpl->uv_stream( ) ) == 0 )
		{
			socket_pimpl->attach_dispatcher( ref->dispatcher_ );

			auto client_socket = tcp_socket::construct(
				std::move( socket_pimpl ) );

			auto writable = ref->channel_->get_writable( );
			if ( !writable.write( std::move( client_socket ) ) )
				ref->close( );

			// TODO: Check writable.should_write( ), and stop
			//       listening for new connections until it
			//       notifies to continue.
		}
		else
		{
			::uv_close( socket_pimpl->uv_handle( ), nullptr );
		}
	};

	int ret = ::uv_listen(
		uv_stream( ), socket_backlog, connection_callback );

	if ( ret )
	{
		// TODO: Convert libuv error to exception
		// TODO: Also do uv_strerror( r ) to get message
		throw_by_errno( uv_error_to_errno( ret ) );
	}

}

} } // namespace io, namespace q
