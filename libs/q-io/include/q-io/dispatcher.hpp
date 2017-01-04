/*
 * Copyright 2014 Gustaf Räntilä
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

#ifndef LIBQIO_DISPATCHER_HPP
#define LIBQIO_DISPATCHER_HPP

#include <q-io/ip.hpp>
#include <q-io/types.hpp>
#include <q-io/udp_types.hpp>

#include <q/event_dispatcher.hpp>
#include <q/promise.hpp>
#include <q/timer.hpp>
#include <q/channel.hpp>
#include <q/block.hpp>
#include <q/options.hpp>
#include <q/backlog.hpp>

namespace q { namespace io {

Q_MAKE_SIMPLE_EXCEPTION( dns_lookup_error );
Q_MAKE_SIMPLE_EXCEPTION( dispatcher_not_running );

enum class dispatcher_termination
{
	graceful,
	immediate
};

enum class dispatcher_exit
{
	normal,
	exited,
	forced,
	failed
};

struct resolver_response
{
	ip_addresses ips;
	// TTL is not always available, and should not be relied on.
	// If it's not available, it will be zero.
	std::chrono::seconds ttl;
};

typedef q::options<
	ip_address,
	q::backlog,
	udp_bind
> udp_receive_options;

typedef q::options<
	ip_address_and_port,
	q::backlog,
	udp_bind
> udp_send_options;

/**
 * The @c dispatcher class is the core execution loop for qio, and forwards
 * control and execution to the underlying event library (libevent).
 */
class dispatcher
: public ::q::event_dispatcher<
	q::arguments< dispatcher_termination >,
	dispatcher_exit
>
, public std::enable_shared_from_this< dispatcher >
, public q::enable_queue_from_this
{
	typedef ::q::event_dispatcher<
		q::arguments< dispatcher_termination >,
		dispatcher_exit
	> event_dispatcher_base;

public:
	struct pimpl;

	struct event_descriptor
	{
		void*       handle;
		const char* type;
		bool        active;
		bool        closing;
		int         fd;
		std::string fd_err;
	};

	/**
	 * Constructs a dispatcher object which handles IO. This function will
	 * likely block for a long time (or until the program ends), so the
	 * event_dispatcher this functions runs on should allow very long
	 * tasks. A reasonable solution would be a threadpool of 1 thread.
	 *
	 * The user_queue is the queue on which callback tasks are invoked put,
	 * such as when IO operations have completed.
	 */
	static dispatcher_ptr construct( q::queue_ptr user_queue,
	                                 std::string name = "q-io dispatcher" );

	~dispatcher( );

	/**
	 * @returns a string describing the backend method used to do I/O
	 * multiplexing.
	 */
	std::string backend_method( ) const;

	/**
	 * @returns The events existing in the dispatcher pool, in a vector of
	 *          `event_descriptor`s.
	 */
	promise< std::vector< event_descriptor > > dump_events( ) const;

	/**
	 * @returns The events existing in the dispatcher pool, as json string.
	 */
	promise< std::string > dump_events_json( ) const;

	/**
	 * Starts the I/O event dispatcher. This will not return until the
	 * dispatcher is terminated using terminate( dispatcher_termination )
	 * or an unmanagable error occurs.
	 */
	void start_blocking( );

	/**
	 * Starts the event dispatcher, as by the contract of the
	 * q::event_dispatcher, this is done non-blocking, i.e. the function
	 * will return immediately, and execution will continue on the
	 * background.
	 */
	promise< > start( ) override;

	/**
	 * Makes a DNS lookup. This is a helper around creating a
	 * q::io::resolver instance with this dispatcher's queue and default
	 * options.
	 */
	q::promise< resolver_response >
	lookup( const std::string& name );

	/**
	 * Connect to a remote peer given a set of ip addresses and a port.
	 */
	q::promise< tcp_socket_ptr >
	get_tcp_connection( ip_addresses&& addresses, std::uint16_t port );

	q::promise< tcp_socket_ptr >
	get_tcp_connection( const ip_addresses& addresses, std::uint16_t port )
	{
		return get_tcp_connection( ip_addresses( addresses ), port );
	}

	/**
	 * Connect to a remote peer given individual ip addresses, and a port.
	 */
	template< typename... Ips >
	typename std::enable_if<
		!std::is_same<
			typename q::arguments< Ips... >::first_type::type,
			typename std::decay< ip_addresses >::type
		>::value,
		q::promise< tcp_socket_ptr >
	>::type
	get_tcp_connection( Ips&&... ips, std::uint16_t port )
	{
		return get_tcp_connection(
			ip_addresses( std::forward< Ips >( ips )... ), port );
	}

	promise< readable< byte_block >, writable< byte_block > >
	tcp_connect( std::string hostname, std::uint16_t port );

	promise< readable< byte_block >, writable< byte_block > >
	tcp_connect( ip_addresses addr, std::uint16_t port );

	promise< readable< byte_block >, writable< byte_block > >
	tcp_connect( ip_address addr, std::uint16_t port )
	{
		return tcp_connect( ip_addresses( std::move( addr ) ), port );
	}

	/**
	 * Create a server_socket which listens to incoming connections on a
	 * certain interface (given its ip address) and port.
	 */
	q::promise< server_socket_ptr >
	listen( std::uint16_t port, ip_addresses&& bind_to );

	q::promise< server_socket_ptr >
	listen(
		std::uint16_t port, std::string bind_to = "0.0.0.0"
	)
	{
		return listen( port, ip_addresses( bind_to ) );
	}

	/**
	 * Open a udp packet for sending data.
	 */
	promise< udp_sender_ptr >
	get_udp_sender(
		ip_address addr,
		std::uint16_t port,
		udp_send_options options = udp_send_options( )
	);

	/**
	 * Open a udp socket for sending data.
	 *
	 * Returns a promise to a `q::writable` of a `q::byte_block` of data
	 * that will be sent to the specified address and port.
	 *
	 * The returned writable manages the socket, meaning that when it is
	 * deleted, the socket is closed properly and removed from the I/O
	 * dispatcher.
	 */
	promise< writable< byte_block > >
	udp_send(
		ip_address addr,
		std::uint16_t port,
		udp_send_options options = udp_send_options( )
	);

	/**
	 * Opens a UDP receiver port, encapsulated in a `udp_receiver`, and
	 * returns it in a promise.
	 *
	 * The options can contain a bind_to `ip_address` and/or a backlog for
	 * how many incoming packets to allow in the readable buffer, before
	 * starting to drop packets.
	 *
	 * NOTE; The default is is infinite backlog, meaning the internal
	 *       readable buffer might consume all memory, so the packets must
	 *       be read quickly, or manually dropped!
	 *
	 * NOTE; `udp_receive` returns a simpler interface.
	 */
	promise< udp_receiver_ptr >
	get_udp_receiver(
		std::uint16_t port,
		udp_receive_options options = udp_receive_options( )
	);

	/**
	 * Opens a UDP receiver port, bound to a certain interface address.
	 *
	 * Returns a promise to a `q::readable` of a `udp_packet` which
	 * contains udp packet data together with the remote address and port.
	 *
	 * The returned readable manages the socket, meaning that when it is
	 * deleted, the socket is closed properly and removed from the I/O
	 * dispatcher.
	 *
	 * The options are the same as in `get_udp_receiver`.
	 */
	promise< readable< udp_packet > >
	udp_receive(
		std::uint16_t port,
		udp_receive_options options = udp_receive_options( )
	);

	/**
	 *
	 */
	q::expect< > await_termination( ) override;

protected:
	dispatcher( q::queue_ptr user_queue, std::string name );

	void do_terminate( dispatcher_termination termination ) override;

private:
	/**
	 * Trigger the event dispatcher to fetch another task
	 */
	void notify( ) override;

	/**
	 * Sets the function which can be called to get a task
	 */
	void set_task_fetcher( ::q::task_fetcher_task&& ) override;


	friend class event;
	friend class resolver;
	friend class tcp_socket;
	friend class udp_receiver;
	friend class udp_sender;
	friend class server_socket;
	friend class timer_task;

	std::shared_ptr< pimpl > pimpl_;
};

} } // namespace io, namespace q

#endif // LIBQIO_DISPATCHER_HPP
