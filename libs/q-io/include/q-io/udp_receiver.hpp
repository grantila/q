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

#ifndef LIBQIO_UDP_RECEIVER_HPP
#define LIBQIO_UDP_RECEIVER_HPP

#include <q-io/ip.hpp>
#include <q-io/types.hpp>
#include <q-io/udp_types.hpp>

#include <q/channel.hpp>
#include <q/block.hpp>

namespace q { namespace io {

/**
 * A socket is a socket connection to a remote peer.
 */
class udp_receiver
: public std::enable_shared_from_this< udp_receiver >
{
public:
	struct pimpl;

	~udp_receiver( );

	/**
	 * Get the incoming channel, to read data from the socket
	 */
	q::readable< udp_packet > get_readable( );

	/**
	 * ?
	 */
	void detach( );

protected:
	static udp_receiver_ptr
	construct( std::shared_ptr< pimpl >&& );

private:
	udp_receiver( std::shared_ptr< pimpl >&& );

	friend class dispatcher;

	template< typename T > friend class q::shared_constructor;

	std::shared_ptr< pimpl > pimpl_;
};

} } // namespace io, namespace q

#endif // LIBQIO_UDP_RECEIVER_HPP
