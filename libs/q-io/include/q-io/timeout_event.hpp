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
 * A timeout_event is a task which will be invoked when a certain duration has
 * expired.
 *
 * The task attached to the timeout_event *must* be noexcept, i.e. guarantee
 * not to throw.
 */
class timeout_event
: public event
, public std::enable_shared_from_this< timeout_event >
{
public:
	static std::shared_ptr< timeout_event >
	construct( q::task task, clock::duration duration );

protected:
	timeout_event( q::task task, clock::duration duration );

private:
	void sub_attach( const dispatcher_ptr& dispatcher ) noexcept override;

	void on_event_timeout( ) noexcept override;

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

} } // namespace io, namespace q

#endif // LIBQIO_TIMEOUT_EVENT_HPP
