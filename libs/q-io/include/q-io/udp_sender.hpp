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

#ifndef LIBQIO_UDP_SENDER_HPP
#define LIBQIO_UDP_SENDER_HPP

#include <q-io/ip.hpp>
#include <q-io/types.hpp>

#include <q/channel.hpp>
#include <q/block.hpp>

namespace q { namespace io {

/**
 * A socket is a socket connection to a remote peer.
 */
class udp_sender
: public std::enable_shared_from_this< udp_sender >
{
public:
	struct pimpl;

	~udp_sender( );

	/**
	 * Get the writable to which data will be sent to the socket
	 */
	q::writable< q::byte_block > get_writable( );

	/**
	 * Close the socket ASAP.
	 *
	 * This will happen when the udp_receiver is destructed unless it is
	 * detached. If it is detached, this will happen when the writables are
	 * closed or destructed.
	 * So usually this need not be done manually.
	 */
	void close( );

	/**
	 * TODO: Describe
	 */
	void detach( );

protected:
	static udp_sender_ptr construct(
		std::shared_ptr< udp_sender::pimpl >&& );

private:
	udp_sender( std::shared_ptr< udp_sender::pimpl >&& );

	friend class dispatcher;

	template< typename T > friend class q::shared_constructor;

	std::shared_ptr< pimpl > pimpl_;
};

} } // namespace io, namespace q

#endif // LIBQIO_UDP_SENDER_HPP
