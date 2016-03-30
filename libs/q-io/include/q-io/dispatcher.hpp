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

#include <q-io/types.hpp>
#include <q-io/ip.hpp>
#include <q-io/dns.hpp>

#include <q/event_dispatcher.hpp>

// ----

#include <q/exception.hpp>
#include <q/mutex.hpp>
#include <q/promise.hpp>
#include <q/timer.hpp>

#include <queue>
#include <atomic>

namespace q { namespace io {

// TODO: Remove these in favor of errno_exceptions in q
Q_MAKE_SIMPLE_EXCEPTION( connection_failed );  // Unknown connection failure
Q_MAKE_SIMPLE_EXCEPTION( connection_refused ); // Connection refused

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

class dispatcher;
typedef std::shared_ptr< dispatcher > dispatcher_ptr;

/**
 * The @c dispatcher class is the core execution loop for qio, and forwards
 * control and execution to the underlying event library (libevent).
 */
class dispatcher
: public ::q::event_dispatcher<
	q::arguments< dispatcher_termination >,
	std::tuple< dispatcher_exit >
>
, ::q::timer_dispatcher
, public std::enable_shared_from_this< dispatcher >
{
	typedef ::q::event_dispatcher<
		q::arguments< dispatcher_termination >,
		std::tuple< dispatcher_exit >
	> event_dispatcher_base;

public:
	struct pimpl;

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
	 * @returns a string describing all currently existing events in the
	 * dispatcher pool.
	 */
	std::string dump_events( ) const;

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
	void start( ) override;

	/**
	 * Attach an event to this dispatcher. The event should not have been
	 * created by this dispatcher, as they are automatically attached.
	 */
	void attach_event( const event_ptr& event );

	/**
	 * Creates a timeout-based forwarding_async_task which can be used to
	 * delay execution in q promise chains.
	 */
	::q::async_task delay( ::q::timer::duration_type dur ) override;

	/**
	 * Makes a DNS lookup. This is a helper around creating a
	 * q::io::resolver instance with this dispatcher's queue and default
	 * options.
	 */
	q::promise< std::tuple< resolver_response > >
	lookup( const std::string& name );

	/**
	 * Connect to a remote peer given a set of ip addresses and a port.
	 */
	q::promise< std::tuple< socket_ptr > >
	connect_to( ip_addresses&& addresses, std::uint16_t port );

	q::promise< std::tuple< socket_ptr > >
	connect_to( const ip_addresses& addresses, std::uint16_t port )
	{
		return connect_to( ip_addresses( addresses ), port );
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
		q::promise< std::tuple< socket_ptr > >
	>::type
	connect_to( Ips&&... ips, std::uint16_t port )
	{
		return connect_to(
			ip_addresses( std::forward< Ips >( ips )... ), port );
	}

	// TODO: Implement 'connect' using domain name lookup (or raw IP
	// addresses if such are provided)

	/**
	 * Create a server_socket which listens to incoming connections on a
	 * certain interface (given its ip address) and port.
	 */
	server_socket_ptr listen( std::uint16_t port );

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

	void _make_dummy_event( );
	void _cleanup_dummy_event( );

	std::unique_ptr< pimpl > pimpl_;
};

} } // namespace io, namespace q

#endif // LIBQIO_DISPATCHER_HPP
