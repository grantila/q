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

#ifndef LIBQ_EVENT_DISPATCHER_HPP
#define LIBQ_EVENT_DISPATCHER_HPP

#include <q/types.hpp>
#include <q/type_traits.hpp>
#include <q/async_termination.hpp>
#include <q/expect.hpp>
#include <q/function.hpp>

#include <memory>

namespace q {

class basic_event_dispatcher;
typedef std::shared_ptr< basic_event_dispatcher > event_dispatcher_ptr;
typedef std::weak_ptr< basic_event_dispatcher > weak_event_dispatcher_ptr;

typedef q::function< timer_task( void ) noexcept > task_fetcher_task;

struct event_dispatcher_yes_tag;
struct event_dispatcher_no_tag;

class custom_queue_from_this
{
public:
	virtual void set_queue( queue_ptr queue ) = 0;
};

class enable_queue_from_this
: public custom_queue_from_this
{
public:
	queue_ptr get_queue( ) const
	{
		return queue_;
	}

	// This must not be called after the queue has been distributed for use
	// since the get_queue() must be thread safe.
	void set_queue( queue_ptr queue )
	{
		queue_ = queue;
	}

private:
	queue_ptr queue_;
};

class basic_event_dispatcher
{
public:
	/**
	 * Trigger the event dispatcher to fetch another task
	 */
	virtual void notify( ) = 0;

	/**
	 * Sets the function which can be called to get a task
	 */
	virtual void set_task_fetcher( task_fetcher_task&& ) = 0;

	/**
	 * Gets the parallelism of this event_dispatcher, or more precisely,
	 * the number of threads this dispatcher is using.
	 */
	virtual std::size_t parallelism( ) const { return 1; }

protected:
	basic_event_dispatcher( )
	{ }
};

enum class termination
{
	/** Wait for backlog to empty out, and allow more tasks while doing so
	 *  but terminate once there is no more jobs */
	linger,

	/** Terminate ASAP, i.e. allow the current task(s) to complete, then
	 *  shutdown, and ignore all other tasks */
	annihilate
};

template<
	typename TerminationArgs = q::arguments< >,
	typename Completion = std::tuple< >
>
class sync_event_dispatcher
: public sync_termination< TerminationArgs >
, public basic_event_dispatcher
{
public:
	~sync_event_dispatcher( )
	{ }

	/**
	 * Starts the event_dispatcher and returns execution immediately, i.e.
	 * this function is non-blocking by contract.
	 */
	virtual void start( ) = 0;

	/**
	 * Awaits the event_dispatcher to have actually terminated. This may be
	 * necessary for some event_dispatchers, which is the reason this
	 * *must* be called after terminate() to ensure the event_dispatcher is
	 * completely finished and cleaned-up.
	 *
	 * NOTE: This method may be blocking!
	 */
	virtual q::expect< > await_termination( ) = 0;

	using sync_termination< TerminationArgs >::do_terminate;

protected:
	sync_event_dispatcher( )
	{ }
};

template<
	typename TerminationArgs = q::arguments< >,
	typename... Completion
>
class event_dispatcher
: public async_termination< TerminationArgs, Completion... >
, public basic_event_dispatcher
{
public:
	~event_dispatcher( )
	{ }

	/**
	 * Start this event_dispatcher. This may require asynchronous work
	 * until the event_dispatcher is ready, which is when the returned
	 * promise is resolved.
	 */
	virtual promise< > start( ) = 0;

	/**
	 * Awaits the event_dispatcher to have actually terminated. This may be
	 * necessary for some event_dispatchers, which is the reason this
	 * *must* be called after terminate() to ensure the event_dispatcher is
	 * completely finished and cleaned-up.
	 *
	 * NOTE: This method may be blocking!
	 */
	virtual q::expect< > await_termination( ) = 0;

	using async_termination< TerminationArgs, Completion... >::do_terminate;

protected:
	event_dispatcher( const queue_ptr& queue )
	: async_termination< TerminationArgs, Completion... >( queue )
	{ }
};

} // namespace q

#endif // LIBQ_EVENT_DISPATCHER_HPP
