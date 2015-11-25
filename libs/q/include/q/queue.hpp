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

#include <memory>

namespace q {

class queue_exception
: public exception
{ };

class queue
: public std::enable_shared_from_this< queue >
{
public:
	typedef std::function< void( std::size_t backlog ) > notify_type;

	static queue_ptr construct( priority_t priority );

	~queue( );

	void push( task&& task );

	priority_t priority( ) const;

	/**
	 * Sets a function callback as consumer of the queue. The queue will call
	 * this function each time a task is added to the queue.
	 *
	 * @returns the number of tasks currently in the queue exactly before the
	 * first call to @c fn will occur.
	 */
	std::size_t set_consumer( notify_type fn );

	bool empty( );

protected:
	queue( priority_t priority = 0 );

private:
	friend class scheduler;

	task pop( );

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

} // namespace q

#endif // LIBQ_QUEUE_HPP
