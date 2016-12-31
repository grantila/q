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

#include "dispatcher.hpp"

#include <q/threadpool.hpp>

#include "unistd.h"

namespace q { namespace io {

std::shared_ptr< dispatcher::pimpl >
dispatcher::pimpl::construct( q::queue_ptr user_queue, std::string name )
{
	auto pimpl = q::make_shared_using_constructor< dispatcher::pimpl >( );

	pimpl->user_queue = user_queue;
	pimpl->name = name;
	pimpl->dummy_event.pipes[ 0 ] = pimpl->dummy_event.pipes[ 1 ] = -1;

	pimpl->dns_context_ = q::make_execution_context< q::threadpool >(
		"q-io dns worker", user_queue, 6 );
	pimpl->dns_queue_ = pimpl->dns_context_->queue( );

	return pimpl;
}

void dispatcher::pimpl::i_create_loop( )
{
	// libuv
	::uv_loop_init( &uv_loop );
}

void dispatcher::pimpl::i_make_dummy_event( )
{
	::pipe( dummy_event.pipes );

	// Pipe
	::uv_pipe_init( &uv_loop, &dummy_event.uv_pipe, 0 );
	::uv_pipe_open( &dummy_event.uv_pipe, dummy_event.pipes[ 0 ] );

	// Generic timer (for all timer events)
	uv_timer_.data = this;
	::uv_timer_init( &uv_loop, &uv_timer_ );

	::uv_async_cb async_callback = [ ]( ::uv_async_t* async )
	{
		auto pimpl = reinterpret_cast< dispatcher::pimpl* >(
			async->data );

		// We'll try to run at most one task this round. We'll schedule
		// this function to be called asap, but giving libuv the
		// possibility to perform other I/O inbetween so that we don't
		// exhaust it with synchronous tasks.
		// The exception is timed tasks, which we'll try to fetch as
		// many as possible, since they don't end up being run here,
		// but from their own timers. Forcing a new loop iteration for
		// just _scheduling_ each timer isn't valuable.

		bool ran_any_task = false;

		while ( true )
		{
			auto task = pimpl->task_fetcher_( );

			if ( !task )
				break;
			else if ( task.is_timed( ) )
				pimpl->i_add_timer_task( std::move( task ) );
			else
			{
				task.task( );
				ran_any_task = true;
				break;
			}
		}

		if ( ran_any_task )
			// Again, we did run a task, there might be more to
			// run, so schedule this again.
			// TODO: Change this so that we try to fetch the next
			//       task and save it in the pimpl, and *only if it
			//       did exist* do we uv_async_send again.
			//       We currently schedule 2 iterations for a
			//       single async task now...
			::uv_async_send( async );
	};

	// Async event for triggering tasks to be performed on the I/O thread
	uv_async.data = this;
	::uv_async_init( &uv_loop, &uv_async, async_callback );
}

void dispatcher::pimpl::i_cleanup_dummy_event( )
{
	if ( true ) // TODO: If ever started
	{
		::uv_close(
			reinterpret_cast< uv_handle_t* >(
				&dummy_event.uv_pipe ),
			nullptr );
		::uv_close(
			reinterpret_cast< uv_handle_t* >( &uv_async ),
			nullptr );

		::uv_timer_stop( &uv_timer_ );
		::uv_close(
			reinterpret_cast< uv_handle_t* >( &uv_timer_ ),
			nullptr );

		::close( dummy_event.pipes[ 0 ] );
		::close( dummy_event.pipes[ 1 ] );
	}
}

void dispatcher::pimpl::i_add_timer_task( q::timer_task task )
{
	timer_tasks_.insert( std::move( task ) );
	i_reschedule_timer( );
}

template< typename Duration >
std::uint64_t in_milliseconds( Duration&& dur )
{
	return std::chrono::duration_cast< std::chrono::milliseconds >( dur )
		.count( );
}

void dispatcher::pimpl::i_reschedule_timer( )
{
	::uv_timer_stop( &uv_timer_ );

	auto iter = timer_tasks_.begin( );
	if ( iter == timer_tasks_.end( ) )
		return;

	uv_timer_cb timer_cb = [ ]( ::uv_timer_t* handle )
	{
		::uv_async_t* async = reinterpret_cast< ::uv_async_t* >(
			handle );

		auto pimpl = reinterpret_cast< dispatcher::pimpl* >(
			async->data );

		auto iter = pimpl->timer_tasks_.begin( );
		if ( iter == pimpl->timer_tasks_.end( ) )
			return; // TODO: Log this - it shouldn't ever happen

		auto now = decltype( iter->wait_until )::clock::now( );
		if ( iter->wait_until <= now )
		{
			q::task task = std::move( iter->task );
			pimpl->timer_tasks_.erase( iter );

			task( );
		}

		pimpl->i_reschedule_timer( );
	};

	auto& task = *iter;

	auto now = decltype( task.wait_until )::clock::now( );

	if ( now >= task.wait_until )
	{
		::uv_timer_start( &uv_timer_, timer_cb, 0, 0 );
	}
	else
	{
		std::uint64_t dur = in_milliseconds( task.wait_until - now );
		::uv_timer_start( &uv_timer_, timer_cb, dur, 0 );
	}
}

static const char* handle_type_name( ::uv_handle_type type )
{
	switch ( type )
	{
		case ::uv_handle_type::UV_ASYNC: return "async";
		case ::uv_handle_type::UV_CHECK: return "check";
		case ::uv_handle_type::UV_FS_EVENT: return "fs_event";
		case ::uv_handle_type::UV_FS_POLL: return "fs_poll";
		case ::uv_handle_type::UV_HANDLE: return "handle";
		case ::uv_handle_type::UV_IDLE: return "idle";
		case ::uv_handle_type::UV_NAMED_PIPE: return "pipe";
		case ::uv_handle_type::UV_POLL: return "poll";
		case ::uv_handle_type::UV_PREPARE: return "prepare";
		case ::uv_handle_type::UV_PROCESS: return "process";
		case ::uv_handle_type::UV_STREAM: return "stream";
		case ::uv_handle_type::UV_TCP: return "tcp";
		case ::uv_handle_type::UV_TIMER: return "timer";
		case ::uv_handle_type::UV_TTY: return "tty";
		case ::uv_handle_type::UV_UDP: return "udp";
		case ::uv_handle_type::UV_SIGNAL: return "signal";
		case ::uv_handle_type::UV_FILE: return "file";
		case ::uv_handle_type::UV_UNKNOWN_HANDLE: return "{unknown}";
		default: return "{invalid type!}";
	}
};

uv_walk_cb event_walker = [ ]( ::uv_handle_t* handle, void* arg )
{
	typedef std::vector< dispatcher::event_descriptor > arg_type;
	auto events = reinterpret_cast< arg_type* >( arg );

	bool is_fd_backed =
		handle->type == ::uv_handle_type::UV_TCP ||
		handle->type == ::uv_handle_type::UV_NAMED_PIPE ||
		handle->type == ::uv_handle_type::UV_TTY ||
		handle->type == ::uv_handle_type::UV_UDP ||
		handle->type == ::uv_handle_type::UV_POLL;

	uv_os_fd_t fd = -1;
	std::string fd_err;
	if ( is_fd_backed )
	{
		auto ret = ::uv_fileno( handle, &fd );
		if ( ret )
		{
			if ( ret == UV_EINVAL )
				fd_err = "UV_EINVAL";
			else if ( ret == UV_EBADF )
				fd_err = "UV_EBADF";
			else
			{
				std::stringstream ss;
				ss << ret;
				fd_err = ss.str( );
			}
			fd = -1;
		}
	}

	events->push_back( dispatcher::event_descriptor{
		handle,
		handle_type_name( handle->type ),
		static_cast< bool >( ::uv_is_active( handle ) ),
		static_cast< bool >( ::uv_is_closing( handle ) ),
		fd,
		std::move( fd_err )
	} );
};

std::vector< dispatcher::event_descriptor >
dispatcher::pimpl::i_dump_events( ) const
{
	std::vector< event_descriptor > events;

	::uv_walk(
		const_cast< ::uv_loop_t* >( &uv_loop ),
		event_walker,
		&events
	);

	return events;
}

} } // namespace io, namespace q
