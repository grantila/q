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

#include <uv.h>

#include <queue>

// This shouldn't conflict with other EV_-constants
#define LIBQ_EV_CLOSE 0x1000

namespace q { namespace io {

struct event::pimpl
{
	std::weak_ptr< dispatcher > dispatcher;
};

struct dispatcher::pimpl
{
	std::shared_ptr< q::thread< void > > thread;

	q::queue_ptr dns_queue_;
	q::specific_execution_context_ptr< q::threadpool > dns_context_;

	struct {
		int pipes[ 2 ];
		::uv_pipe_t uv_pipe;
	} dummy_event;

	q::queue_ptr user_queue;
	std::string name;

	::uv_loop_t uv_loop;
	::uv_async_t uv_async;

	std::queue< q::task > tasks_;

	::q::task_fetcher_task task_fetcher_;
};

/*
struct socket_event::pimpl
{
	::uv_tcp_t socket_;
	::uv_connect_t connect_;

	std::atomic< bool > closed_;
};
*/

struct socket::pimpl
{
std::string debug;

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

	void close( );
	void close( std::exception_ptr err );
	void close( expect< void > status );

	void start_read( );
	void stop_read( bool reschedule = false );

	void begin_write( );
};

struct server_socket::pimpl
{
	event::pimpl event_;

	std::shared_ptr< q::channel< socket_ptr > > channel_;

	std::uint16_t port_;
	ip_addresses bind_to_;

	dispatcher_ptr dispatcher_;

	::uv_loop_t* uv_loop_;

	::uv_tcp_t socket_;

	pimpl( )
	{
		std::cout << "CONSTRUCTING socket_server::pimpl" << std::endl;
	}
	~pimpl( )
	{
		std::cout << "DESTRUCTING socket_server::pimpl" << std::endl;
	}
};

struct timer_task::pimpl
{
	event::pimpl event_;

	::uv_loop_t* loop_;
	::uv_timer_t timer_;

	std::shared_ptr< pimpl > cleanup_keepalive_ptr_;

	dispatcher_ptr dispatcher_;
	std::shared_ptr< q::task > task_;
	clock::duration duration_;
	clock::duration repeat_;

	pimpl( )
	: loop_( nullptr )
	, duration_( clock::duration( 0 ) )
	, repeat_( clock::duration( 0 ) )
	{ }
};

// TODO: Properly implement this for Win32
static inline int uv_error_to_errno( int errnum )
{
	return -errnum;
}

} } // namespace io, namespace q

#endif // LIBQIO_INTERNAL_INTERNALS_HPP
