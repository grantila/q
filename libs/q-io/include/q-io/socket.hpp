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

#ifndef LIBQIO_SOCKET_HPP
#define LIBQIO_SOCKET_HPP

#include <q-io/types.hpp>
#include <q-io/socket_event.hpp>
#include <q-io/ip.hpp>

#include <q/channel.hpp>
#include <q/block.hpp>

namespace q { namespace io {

/**
 * A socket is a socket connection to a remote peer.
 */
class socket
: public std::enable_shared_from_this< socket >
, public socket_event
{
public:
	~socket( );

	/**
	 * Get the incoming channel, to read data from the socket
	 */
	q::readable< q::byte_block > in( );

	/**
	 * Get the outoming channel, to write data to the socket
	 */
	q::writable< q::byte_block > out( );

	/**
	 * Makes this socket become owned by its channels. The user can thereby
	 * delete its last reference to this socket, and rely on the channels
	 * to ensure the socket isn't deleted prematurely.
	 *
	 * When both channels are closed, and all outoing data on the writable
	 * channel is written to the socket, the channels will remove their
	 * references to the socket and it will be destructed/deleted if they
	 * held the last references.
	 *
	 * By default, the socket is owning the channels, and if the user
	 * removes all its references to the socket, it will be deleted, and
	 * both channels will be closed and deleted too (unless the user has
	 * further references to them).
	 */
	void detach( );

protected:
	static socket_ptr construct( socket_t );

private:
	socket( socket_t );

	friend class dispatcher;
	friend class server_socket;

	template< typename T > friend class q::shared_constructor;
/*
	std::shared_ptr< q::channel< q::byte_block > > _in( );
	std::shared_ptr< q::channel< q::byte_block > > _out( );
*/

	socket_event_ptr socket_event_shared_from_this( ) override;

	void on_attached( const dispatcher_ptr& dispatcher ) noexcept override;

	void on_event_read( ) noexcept override;
	void on_event_write( ) noexcept override;

	void try_write( );

	void close_socket( );

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

} } // namespace io, namespace q

#endif // LIBQIO_SOCKET_HPP
