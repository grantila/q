/*
 * Copyright 2013 Gustaf Räntilä
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

#ifndef LIBQ_THREADPOOL_HPP
#define LIBQ_THREADPOOL_HPP

#include <q/event_dispatcher.hpp>
#include <q/thread.hpp>

namespace q {

class threadpool
: public event_dispatcher< q::arguments< termination > >
, public std::enable_shared_from_this< threadpool >
{
public:
	~threadpool( );

	void notify( ) override;

	void set_task_fetcher( task_fetcher_task&& ) override;

	static std::shared_ptr< threadpool >
	construct( const std::string& name,
	           const queue_ptr& queue,
	           std::size_t threads = hard_cores( ) );

	q::expect< > await_termination( ) override;

	/**
	 * Starts the threadpool. Ensure the scheduler is attached (with
	 * queue(s)), since the threads will start consuming from the scheduler
	 * immediately.
	 */
	void start( ) override;

protected:
	threadpool( const std::string& name,
	            const queue_ptr& queue,
	            std::size_t threads );

private:

	void mark_completion( );

	void do_terminate( termination ) override;

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

} // namespace q

#endif // LIBQ_THREADPOOL_HPP
