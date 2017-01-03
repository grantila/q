/*
 * Copyright 2017 Gustaf Räntilä
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

#ifndef LIBQIO_INTERNAL_IMPL_UDP_SENDER_HPP
#define LIBQIO_INTERNAL_IMPL_UDP_SENDER_HPP

#include "handle.hpp"

#include <q-io/udp_sender.hpp>

namespace q { namespace io {

struct udp_sender::pimpl
: handle
, std::enable_shared_from_this< udp_sender::pimpl >
{
	using handle::close;

	typedef udp_sender::pimpl* data_ref_type;

	static std::shared_ptr< udp_sender::pimpl >
	construct(
		queue_ptr internal_queue,
		ip_address addr,
		std::uint16_t port,
		udp_send_options options
	);

	std::shared_ptr< pimpl > keep_alive_;

	std::shared_ptr< q::readable< ::q::byte_block > > readable_out_; // Int
	std::shared_ptr< q::writable< ::q::byte_block > > writable_out_; // Ext

	std::unique_ptr< udp_send_options > construction_options_;

	std::atomic< bool > closed_;
	std::atomic< bool > detached_;

	::uv_udp_t udp_;

	std::shared_ptr< ::sockaddr > sockaddr_;
	bool is_infinite_;

	// Caches necessary for libuv
	struct write_info
	{
		std::shared_ptr< pimpl > keep_alive_;
		::uv_udp_send_t req_;
		byte_block block_; // Ensures we keep the memory while sending
		std::size_t buf_len_;
	};
	std::deque< std::unique_ptr< write_info > > write_reqs_;

	void
	attach_dispatcher( const dispatcher_ptr& dispatcher ) noexcept override;

	void close( expect< void > status ) override;

	void send_block( ::q::byte_block block );
	void read_write_one( );

	void detach( );

protected:
	pimpl( )
	: handle( reinterpret_cast< ::uv_handle_t* >( &udp_ ) )
	, closed_( false )
	, detached_( false )
	, sockaddr_( )
	, is_infinite_( false )
	{
		udp_.data = nullptr;
		udp_.loop = nullptr;
	}
};

} } // namespace io, namespace q

#endif // LIBQIO_INTERNAL_IMPL_UDP_SENDER_HPP
