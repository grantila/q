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

#include <memory>

namespace q {

class event_dispatcher;
typedef std::shared_ptr< event_dispatcher > event_dispatcher_ptr;

class event_dispatcher
{
public:
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

	~event_dispatcher( )
	{ }

	virtual void add_task( task ) = 0;

	/**
	 * TODO: Reconsider
	 */
	virtual std::size_t backlog( ) const = 0;

protected:
	event_dispatcher( )
	{ }
};

} // namespace q

#endif // LIBQ_EVENT_DISPATCHER_HPP
