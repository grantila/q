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

#ifndef LIBQ_PROMISE_SIGNAL_HPP
#define LIBQ_PROMISE_SIGNAL_HPP

#include <q/types.hpp>
#include <q/mutex.hpp>
#include <q/small_vector.hpp>

#include <memory>

namespace q { namespace detail {

// TODO: Make lock-free with a lock-free queue and atomic bool.
class promise_signal
{
public:
	/**
	 * items are tasks bound to a certain queue.
	 *
	 * They can also be synchronous tasks which run directly when the
	 * promise is resolved. These must be tiny and fast and they must be
	 * 'noexcept'. This is used for scheduling custom async tasks for event
	 * loops, e.g. timers.
	 */
	struct item
	{
		task task_;
		queue_ptr queue_;

		item( task&& task, queue_ptr&& queue_ )
		: task_( std::move( task ) )
		, queue_( std::move( queue_ ) )
		{ }

		item( task&& task )
		: task_( std::move( task ) )
		{ }
	};

	promise_signal( )
	: done_( false )
	{ }

	void notify( ) noexcept;
	void push( task&& task, queue_ptr queue ) noexcept;
	void push_synchronous( task&& task ) noexcept;

	mutex mutex_;
	bool done_;
	std::vector< item > items_;
};

typedef std::shared_ptr< promise_signal > promise_signal_ptr;

} } // namespace detail, namespace queue

#endif // LIBQ_PROMISE_SIGNAL_HPP
