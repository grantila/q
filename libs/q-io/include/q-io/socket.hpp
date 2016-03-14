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
#include <q-io/event.hpp>
#include <q-io/ip.hpp>

#include <q/channel.hpp>
#include <q/block.hpp>

namespace q { namespace io {

/**
 * A socket is a socket connection to a remote peer.
 */
class socket
: public event
, public std::enable_shared_from_this< socket >
{
public:
	~socket( );

	/**
	 * Get the incoming channel, to read data from the socket
	 */
	q::readable_ptr< q::byte_block > in( );

	/**
	 * Get the outoming channel, to write data to the socket
	 */
	q::writable_ptr< q::byte_block > out( );

protected:
	static socket_ptr construct( const native_socket& );

private:
	socket( const native_socket& );

	friend class dispatcher;
	friend class server_socket;

	template< typename T > friend class q::shared_constructor;

	void sub_attach( const dispatcher_ptr& dispatcher ) noexcept override;

	void on_event_read( ) noexcept override;
	void on_event_write( ) noexcept override;

	void try_write( );

	void close_socket( );
	void close_reader( );
	void close_writer( );

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

} } // namespace io, namespace q

#endif // LIBQIO_SOCKET_HPP
