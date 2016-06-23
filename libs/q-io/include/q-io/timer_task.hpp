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

#ifndef LIBQIO_TIMER_TASK_HPP
#define LIBQIO_TIMER_TASK_HPP

#include <q-io/clock.hpp>
#include <q-io/event.hpp>
#include <q-io/types.hpp>

namespace q { namespace io {

/**
 * A timer_task is a re-usable object which will invoke a task after a certain
 * duration, potentially with a certain repeat interval.
 * It can be restarted with a new duration (and repeat interval) and can also
 * replace the task to be invoked with another task.
 *
 * The task attached to the timer_task *must* guarantee not to throw.
 */
class timer_task
: public event
{
	struct pimpl;
public:
	timer_task( );
	timer_task( const timer_task& ) = delete;
	timer_task( timer_task&& ) = default;
	~timer_task( );

	void set_task( q::task task );
	void unset_task( );

	void start_timeout(
		clock::duration,
		clock::duration repeat = clock::duration( 0 )
	);

	void stop( );

private:
	void sub_attach( const dispatcher_ptr& dispatcher ) noexcept override;

	bool is_attached( ) const;
	void ensure_attached( ) const;

	std::shared_ptr< pimpl > pimpl_;
};

} } // namespace io, namespace q

#endif // LIBQIO_TIMER_TASK_HPP
