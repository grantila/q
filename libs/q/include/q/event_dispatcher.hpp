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

#include <memory>

namespace q {

class basic_event_dispatcher;
typedef std::shared_ptr< basic_event_dispatcher > event_dispatcher_ptr;

class basic_event_dispatcher
{
public:
	/**
	 * Runs a task as soon as possible
	 */
	virtual void add_task( task ) = 0;

protected:
	basic_event_dispatcher( )
	{ }
};

enum class termination
{
	/** Wait for backlog to empty out, and allow more tasks while doing so
	 *  but terminate once there is no more jobs */
	linger,

	/** Disallow further tasks, but complete all current tasks in the
	 *  backlog */
	process_backlog,

	/** Terminate ASAP, i.e. allow the current task(s) to complete, then
	 *  shutdown, and ignore all other tasks */
	annihilate
};

template<
	typename TerminationArgs = q::arguments< >,
	typename Completion = std::tuple< >
>
class event_dispatcher
: public sync_termination< TerminationArgs >
, public basic_event_dispatcher
{
public:
	~event_dispatcher( )
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
	event_dispatcher( )
	{ }
};

template<
	typename TerminationArgs = q::arguments< >,
	typename Completion = std::tuple< >
>
class async_event_dispatcher
: public async_termination< TerminationArgs, Completion >
, public basic_event_dispatcher
{
public:
	~async_event_dispatcher( )
	{ }

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

	using async_termination< TerminationArgs, Completion >::do_terminate;

protected:
	async_event_dispatcher( const queue_ptr& queue )
	: async_termination< TerminationArgs, Completion >( queue )
	{ }
};

} // namespace q

#endif // LIBQ_EVENT_DISPATCHER_HPP
