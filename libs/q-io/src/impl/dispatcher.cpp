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

	// libuv
	::uv_loop_init( &pimpl->uv_loop );

	pimpl->make_dummy_event( );

	return pimpl;
}

void dispatcher::pimpl::make_dummy_event( )
{
	::pipe( dummy_event.pipes );

	// Pipe
	::uv_pipe_init( &uv_loop, &dummy_event.uv_pipe, 0 );
	::uv_pipe_open( &dummy_event.uv_pipe, dummy_event.pipes[ 0 ] );

	::uv_async_cb async_callback = [ ]( ::uv_async_t* async )
	{
		auto pimpl = reinterpret_cast< dispatcher::pimpl* >(
			async->data );

		auto task = pimpl->task_fetcher_( );
		if ( !task )
			return;

		task.task( );

		// Retry with another task if its available. We could do it
		// in-place here, but instead we'll asynchronously do it,
		// meaning we'll schedule this function to be called asap, but
		// giving libuv the possibility to perform other I/O inbetween.
		::uv_async_send( async );
	};

	// Async event for triggering tasks to be performed on the I/O thread
	uv_async.data = this;
	::uv_async_init( &uv_loop, &uv_async, async_callback );
}

void dispatcher::pimpl::cleanup_dummy_event( )
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

		::close( dummy_event.pipes[ 0 ] );
		::close( dummy_event.pipes[ 1 ] );
	}
}

} } // namespace io, namespace q
