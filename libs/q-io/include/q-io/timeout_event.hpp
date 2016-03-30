/*
 * Copyright 2014 Gustaf Räntilä
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

#ifndef LIBQIO_TIMEOUT_EVENT_HPP
#define LIBQIO_TIMEOUT_EVENT_HPP

#include <q-io/types.hpp>
#include <q-io/clock.hpp>
#include <q-io/dispatcher.hpp>
#include <q-io/event.hpp>

namespace q { namespace io {

/**
 * A timeout_event is an event which can be used multiple times to trigger
 * timeouts.
 *
 * The class is abstract, and requires a sub-class to override the
 * `on_event_timeout( )` function.
 *
 * The overridden `on_event_timeout` *must* be noexcept, i.e. guarantee not to
 * throw.
 */
class timeout_event
: public event
, public std::enable_shared_from_this< timeout_event >
{
public:
	virtual ~timeout_event( );

	void set_timeout_now( );
	void set_timeout( clock::duration duration );
	void remove_timeout( );

protected:
	timeout_event( );

	virtual void on_event_timeout( ) noexcept override = 0;
	virtual void on_initialized( ) noexcept { }

private:
	void sub_attach( const dispatcher_ptr& dispatcher ) noexcept override;

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

/**
 * A timeout_task is a task which will be invoked when a certain duration has
 * expired. It is a one-time class which must not be re-used.
 *
 * The task attached to the timeout_event *must* be noexcept, i.e. guarantee
 * not to throw.
 */
class timeout_task
: public timeout_event
{
public:
	static std::shared_ptr< timeout_task >
	construct( q::task task, clock::duration duration );

	~timeout_task( );

protected:
	timeout_task( q::task task, clock::duration duration );

private:
	void on_event_timeout( ) noexcept override;
	void on_initialized( ) noexcept override;

	q::task task_;
	clock::duration duration_;
	std::shared_ptr< timeout_task > self_ptr_;
};

} } // namespace io, namespace q

#endif // LIBQIO_TIMEOUT_EVENT_HPP
