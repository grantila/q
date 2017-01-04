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

#ifndef LIBQIO_INTERNAL_IMPL_TCP_SOCKET_HPP
#define LIBQIO_INTERNAL_IMPL_TCP_SOCKET_HPP

#include "stream.hpp"

#include <q-io/tcp_socket.hpp>

namespace q { namespace io {

struct tcp_socket::pimpl
: stream
, std::enable_shared_from_this< tcp_socket::pimpl >
{
	using handle::i_close;

	typedef std::shared_ptr< tcp_socket::pimpl > data_ref_type;

	static std::shared_ptr< tcp_socket::pimpl > construct( );

	std::shared_ptr< dispatcher::pimpl > dispatcher_;
	std::weak_ptr< pimpl > self_;

	std::shared_ptr< q::readable< q::byte_block > > readable_in_; // Ext
	std::shared_ptr< q::writable< q::byte_block > > writable_in_; // Int
	std::shared_ptr< q::readable< q::byte_block > > readable_out_; // Int
	std::shared_ptr< q::writable< q::byte_block > > writable_out_; // Ext

	std::atomic< bool > can_read_;
	std::atomic< bool > can_write_;
	q::byte_block out_buffer_;

	std::atomic< bool > closed_;

	::uv_tcp_t socket_;
	::uv_connect_t connect_;

	// Caches necessary for libuv
	static const std::size_t cache_size = 64 * 1024;
	typedef std::shared_ptr< pimpl > write_req_self_ptr;
	// Decremented from libuv-thread, incremented form receive()-thread
	std::size_t cached_bytes_;
	struct write_info
	{
		std::unique_ptr< ::uv_write_t > req_;
		byte_block block_; // Ensures we keep the memory while sending
		std::size_t buf_len_;
	};
	std::deque< write_info > write_reqs_;

	void
	i_attach_dispatcher( const dispatcher_pimpl_ptr& dispatcher )
	noexcept override;

	void i_close( expect< void > status ) override;

	void start_read( );
	void stop_read( bool reschedule = false );

	void begin_write( );

protected:
	pimpl( )
	: stream( reinterpret_cast< ::uv_stream_t* >( &socket_ ) )
	, can_read_( false )
	, can_write_( false )
	, closed_( false )
	, cached_bytes_( 0 )
	{
		socket_.data = nullptr;
		socket_.loop = nullptr;
	}
};

} } // namespace io, namespace q

#endif // LIBQIO_INTERNAL_IMPL_TCP_SOCKET_HPP
