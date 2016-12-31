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

#ifndef LIBQ_QUEUE_HPP
#define LIBQ_QUEUE_HPP

#include <q/types.hpp>
#include <q/mutex.hpp>
#include <q/exception.hpp>
#include <q/timer.hpp>

#include <memory>

namespace q {

class queue_exception
: public exception
{ };

struct timer_task
{
	timer_task( ) = default;
	timer_task( timer_task&& ) = default;
	timer_task( const timer_task& ) = delete;

	timer_task( task&& _task )
	: task( std::move( _task ) )
	, is_timed_( false )
	{ }

	timer_task( task&& _task, timer::point_type&& _wait_until )
	: task( std::move( _task ) )
	, wait_until( std::move( _wait_until ) )
	, is_timed_( true )
	{ }

	bool operator!( ) const
	{
		return !this->task;
	}

	operator bool( ) const
	{
		return !!this->task;
	}

	bool operator<( const timer_task& other ) const
	{
		return wait_until < other.wait_until;
	}

	bool is_timed( ) const
	{
		return is_timed_;
	}

	task task;
	timer::point_type wait_until;

private:
	bool is_timed_;
};

class queue
: public std::enable_shared_from_this< queue >
{
public:
	typedef timer_task element_type;
	typedef q::function< void( ) > notify_type;

	static queue_ptr construct( priority_t priority );

	~queue( );

	void push( task&& task );
	void push( task&& task, timer::point_type wait_until );

	priority_t priority( ) const;

	/**
	 * Sets a function callback as consumer of the queue. The queue will
	 * call this function each time a task is added to the queue.
	 */
	void set_consumer( notify_type fn, std::size_t parallelism );

	bool empty( );

	timer_task pop( );

	std::size_t parallelism( ) const;

protected:
	queue( priority_t priority = 0 );

private:
	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

} // namespace q

#endif // LIBQ_QUEUE_HPP
