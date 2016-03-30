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

#include <q-io/event.hpp>
#include <q-io/dispatcher.hpp>
#include <q-io/dns.hpp>

#include <q/thread.hpp>

#include <event2/event.h>
#include <event2/dns.h>

// This shouldn't conflict with other EV_-constants
#define LIBQ_EV_CLOSE 0x1000

namespace q { namespace io {

struct native_socket
{
	evutil_socket_t fd;
};

struct event::pimpl
{
	struct ::event* ev;
	socket_t fd;

	std::weak_ptr< dispatcher > dispatcher;
};

struct dispatcher::pimpl
{
	std::shared_ptr< q::thread< void > > thread;

	struct {
		::event* ev;
		int pipes[ 2 ];
	} dummy_event;

	q::queue_ptr            user_queue;
	std::string             name;

	// libevent2 types
	::event_base*           event_base;

	std::queue< q::task > tasks_;

	::q::task_fetcher_task task_fetcher_;

	pimpl( )
	: event_base( nullptr )
	{ }
};

struct resolver::pimpl
{
	dispatcher_ptr dispatcher_;
	::evdns_base* base_;
};

} } // namespace io, namespace q
