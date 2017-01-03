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

#ifndef LIBQIO_INTERNAL_IMPL_UDP_RECEIVER_HPP
#define LIBQIO_INTERNAL_IMPL_UDP_RECEIVER_HPP

#include "handle.hpp"

#include <q-io/udp_receiver.hpp>

namespace q { namespace io {

struct udp_receiver::pimpl
: handle
, std::enable_shared_from_this< udp_receiver::pimpl >
{
	using handle::close;

	typedef std::shared_ptr< udp_receiver::pimpl > data_ref_type;

	static std::shared_ptr< udp_receiver::pimpl >
	construct(
		queue_ptr user_queue,
		std::uint16_t port,
		udp_receive_options options
	);

	std::weak_ptr< pimpl > self_;

	std::shared_ptr< q::readable< udp_packet > > readable_in_; // Ext
	std::shared_ptr< q::writable< udp_packet > > writable_in_; // Int

	std::unique_ptr< udp_receive_options > construction_options_;

	std::atomic< bool > can_read_;
	std::atomic< bool > closed_;
	std::atomic< bool > detached_;

	::uv_udp_t udp_;

	std::uint16_t port_;
	bool is_infinite_; // Whether we should care about back pressure

	void
	attach_dispatcher( const dispatcher_ptr& dispatcher ) noexcept override;

	void close( expect< void > status ) override;

	void start_read( );
	void stop_read( bool reschedule = false );

	void detach( );

protected:
	pimpl( )
	: handle( reinterpret_cast< ::uv_handle_t* >( &udp_ ) )
	, can_read_( false )
	, closed_( false )
	, detached_( false )
	, port_( 0 )
	, is_infinite_( false )
	{
		udp_.data = nullptr;
		udp_.loop = nullptr;
	}
};

} } // namespace io, namespace q

#endif // LIBQIO_INTERNAL_IMPL_UDP_RECEIVER_HPP
