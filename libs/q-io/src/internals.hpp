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

#ifndef LIBQIO_INTERNAL_INTERNALS_HPP
#define LIBQIO_INTERNAL_INTERNALS_HPP

#include <q-io/event.hpp>
#include <q-io/dispatcher.hpp>
#include <q-io/dns.hpp>
#include <q-io/socket.hpp>
#include <q-io/server_socket.hpp>
#include <q-io/timer_task.hpp>

#include <q/thread.hpp>
#include <q/execution_context.hpp>

#include <event2/event.h>
#include <event2/dns.h>

#include <uv.h>

#include <queue>

// This shouldn't conflict with other EV_-constants
#define LIBQ_EV_CLOSE 0x1000

namespace q { namespace io {

struct event::pimpl
{
#ifdef QIO_USE_LIBEVENT
	struct ::event* ev;
	socket_t fd;
#endif

	std::weak_ptr< dispatcher > dispatcher;
};

struct dispatcher::pimpl
{
	std::shared_ptr< q::thread< void > > thread;

#ifndef QIO_USE_LIBEVENT_DNS
	q::queue_ptr dns_queue_;
	q::specific_execution_context_ptr< q::threadpool > dns_context_;
#endif

	struct {
		int pipes[ 2 ];
#ifdef QIO_USE_LIBEVENT
		::event* ev;
#else
		::uv_pipe_t uv_pipe;
#endif
	} dummy_event;

	q::queue_ptr user_queue;
	std::string name;

#ifdef QIO_USE_LIBEVENT
	// libevent2 types
	::event_base*           event_base;
#else
	// libuv
	::uv_loop_t uv_loop;
	::uv_async_t uv_async;
#endif

	std::queue< q::task > tasks_;

	::q::task_fetcher_task task_fetcher_;

	pimpl( )
#ifdef QIO_USE_LIBEVENT
	: event_base( nullptr )
#endif
	{ }
};

/*
struct socket_event::pimpl
{
#ifdef QIO_USE_LIBEVENT
	socket_t fd_;

	::event* ev_read_;
	::event* ev_write_;
#else
	::uv_tcp_t socket_;
	::uv_connect_t connect_;
#endif

	std::atomic< bool > closed_;

#ifdef QIO_USE_LIBEVENT
	pimpl( socket_t sock )
	: fd_( sock )
	, ev_read_( nullptr )
	, ev_write_( nullptr )
	, closed_( false )
	{ }
#endif
};
*/

struct socket::pimpl
{
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
};

struct server_socket::pimpl
{
	std::shared_ptr< q::channel< socket_ptr > > channel_;

	std::uint16_t port_;
	ip_addresses bind_to_;

	dispatcher_ptr dispatcher_;

	::uv_loop_t* uv_loop_;

#ifdef QIO_USE_LIBEVENT
	socket_t socket_;
	std::size_t backlog_;
	::event* ev_;
#else
	::uv_tcp_t socket_;
#endif

#ifdef QIO_USE_LIBEVENT
	std::atomic< bool > can_read_;

	server_socket_ptr self_;

	std::atomic< bool > closed_;
#endif
};

struct timer_task::pimpl
{
	::uv_loop_t* loop_;
	::uv_timer_t timer_;

	std::shared_ptr< pimpl > cleanup_keepalive_ptr_;

	std::shared_ptr< dispatcher::pimpl > dispatcher_pimpl_;
	std::shared_ptr< q::task > task_;
	clock::duration duration_;
	clock::duration repeat_;

	pimpl( )
	: loop_( nullptr )
	, duration_( clock::duration( 0 ) )
	, repeat_( clock::duration( 0 ) )
	{ }
};

/*
struct resolver::pimpl
{
	dispatcher_ptr dispatcher_;
	::evdns_base* base_;
};
*/

// TODO: Properly implement this for Win32
static int uv_error_to_errno( int errnum )
{
	return -errnum;
}

} } // namespace io, namespace q

#endif // LIBQIO_INTERNAL_INTERNALS_HPP
