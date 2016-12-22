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

namespace q { namespace io {

struct socket::pimpl : stream
{
std::string debug;

	pimpl( )
	: stream( reinterpret_cast< ::uv_stream_t* >( &socket_ ) )
	{ }

	~pimpl( )
	{
		std::cout << "DESTRUCTING pimpl " << debug << std::endl;
	}

	std::weak_ptr< pimpl > self_;

	event::pimpl event_;

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
	q::mutex mut_;
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

	void close( ) override;
	void close( std::exception_ptr err );
	void close( expect< void > status );

	void start_read( );
	void stop_read( bool reschedule = false );

	void begin_write( );
};

} } // namespace io, namespace q

#endif // LIBQIO_INTERNAL_IMPL_TCP_SOCKET_HPP
