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

void server_socket::pimpl::i_close( q::expect< void > exp )
{
	i_close_handle( );
}

void server_socket::pimpl::i_attach_dispatcher(
	const dispatcher_pimpl_ptr& dispatcher
) noexcept
{
	dispatcher_ = dispatcher;

	const int channel_backlog = 32; // TODO: Reconsider backlog
	const int socket_backlog = 64;  // TODO: Reconsider backlog

	channel_ = q::make_unique< q::channel< tcp_socket_ptr > >(
		dispatcher_->user_queue, channel_backlog );

	auto iter = bind_to_.begin( );

	if ( iter == bind_to_.end( ) )
		Q_THROW( invalid_ip_address( ) );

	auto addr = iter->get_sockaddr( port_ );

	::uv_tcp_init( &dispatcher_->uv_loop, &socket_ );

	::uv_tcp_bind( &socket_, &*addr, 0 );

	uv_connection_cb connection_callback =
	[ ]( ::uv_stream_t* server, int status )
	{
		auto pimpl = get_pimpl< server_socket::pimpl >( server );

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

		if ( !pimpl )
		{
			// We have just removed this server listener when an
			// incoming socket came in. Just ignore it.
			// This should probably never happen.
			return;
		}

		auto socket_pimpl = q::make_shared< tcp_socket::pimpl >( );

		::uv_tcp_init(
			&pimpl->dispatcher_->uv_loop, &socket_pimpl->socket_ );

		if ( ::uv_accept( server, socket_pimpl->uv_stream( ) ) == 0 )
		{
			socket_pimpl->i_attach_dispatcher( pimpl->dispatcher_ );

			auto client_socket = tcp_socket::construct(
				std::move( socket_pimpl ) );

			auto writable = pimpl->channel_->get_writable( );
			if ( !writable.write( std::move( client_socket ) ) )
				pimpl->i_close( );

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

	keep_alive_ = shared_from_this( );
}

} } // namespace io, namespace q
