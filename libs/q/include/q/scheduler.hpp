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

#ifndef LIBQ_SCHEDULER_HPP
#define LIBQ_SCHEDULER_HPP

#include <q/event_dispatcher.hpp>
#include <q/queue.hpp>

#include <memory>

namespace q {

class scheduler;
typedef std::shared_ptr< scheduler > scheduler_ptr;

class scheduler
{
public:
	virtual void add_queue( queue_ptr queue ) = 0;

protected:
	scheduler( ) { }
	scheduler( const scheduler& ) = delete;
	scheduler( scheduler&& ) = delete;
	virtual ~scheduler( ) { }
};

/**
 * priority_scheduler is a priority queue backed scheduler, prividing the
 * possibility for multiple queues, with or without different priorities.
 *
 * Attached queues with equal priotiy will be scheduled in a round-robin
 * distribution, and queues with higher priority will always be scheduled
 * before lower-priority queues.
 *
 * There is no logic for handling starvation or fairness of any kind.
 */
class priority_scheduler
: public scheduler
, public std::enable_shared_from_this< priority_scheduler >
{
public:
	~priority_scheduler( );

	void add_queue( queue_ptr queue );

protected:
	priority_scheduler( const event_dispatcher_ptr& event_dispatcher );

private:
	timer_task next_task( );

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

Q_MAKE_SIMPLE_EXCEPTION( not_unique_exception );

/**
 * direct_scheduler is a fast lock-free scheduler that allows only one queue,
 * which cannot be changed at run-time.
 *
 * Dispatching of tasks will go straight from the queue to the underlying
 * event_dispatcher without any logic whatsoever.
 *
 * This is the recommended scheduler to use, unless support for prioritizing
 * queues is necessary.
 */
class direct_scheduler
: public scheduler
, public std::enable_shared_from_this< direct_scheduler >
{
public:
	~direct_scheduler( );

	/**
	 * Will always throw q::not_unique_exception if called more than once.
	 */
	void add_queue( queue_ptr queue );

protected:
	direct_scheduler( const event_dispatcher_ptr& event_dispatcher );

private:
	timer_task next_task( );

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

} // namespace q

#endif // LIBQ_SCHEDULER_HPP
