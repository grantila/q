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

#ifndef LIBQIO_INTERNAL_IMPL_DISPATCHER_HPP
#define LIBQIO_INTERNAL_IMPL_DISPATCHER_HPP

#include <q-io/dispatcher.hpp>

#include <q/thread.hpp>
#include <q/execution_context.hpp>

#include <set>

#include "../uv.hpp"

namespace q { namespace io {

struct dispatcher::pimpl
{
	std::shared_ptr< q::thread< void > > thread;

	q::queue_ptr dns_queue_;
	q::specific_execution_context_ptr< q::threadpool > dns_context_;

	// Refactor into _real_ pipe object
	struct {
		int pipes[ 2 ];
		::uv_pipe_t uv_pipe;
	} dummy_event;

	q::queue_ptr user_queue;
	std::string name;
	std::shared_ptr< q::detail::defer< > > deferred_start_;

	::uv_loop_t uv_loop;
	::uv_async_t uv_async; // TODO: Consider refactoring to async object
	::uv_timer_t uv_timer_;
	std::set< q::timer_task > timer_tasks_;
	std::atomic< bool > started_;
	std::atomic< bool > stopped_;

	::q::task_fetcher_task task_fetcher_;

	std::atomic< dispatcher_termination > termination_;

	static std::shared_ptr< pimpl >
	construct( q::queue_ptr user_queue, std::string name );

	void i_create_loop( );
	void i_make_dummy_event( );
	void i_cleanup_dummy_event( );

	void i_add_timer_task( q::timer_task task );
	void i_reschedule_timer( );

	std::vector< dispatcher::event_descriptor > i_dump_events( ) const;

protected:
	pimpl( )
	: started_( false )
	, stopped_( false )
	{ }
};

} } // namespace io, namespace q

#endif // LIBQIO_INTERNAL_IMPL_DISPATCHER_HPP
