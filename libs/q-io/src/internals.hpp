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

#ifndef LIBQIO_INTERNAL_INTERNALS_HPP
#define LIBQIO_INTERNAL_INTERNALS_HPP

#include <q-io/event.hpp>
#include <q-io/dispatcher.hpp>
#include <q-io/dns.hpp>

#include <q/thread.hpp>
#include <q/execution_context.hpp>

#include <event2/event.h>
#include <event2/dns.h>

#include <queue>

// This shouldn't conflict with other EV_-constants
#define LIBQ_EV_CLOSE 0x1000

namespace q { namespace io {

struct callback_arg_type
{
	enum class event_type
	{
		timer,
		socket_event,
		server_socket
	} type;
};

struct socket_arg_type
: callback_arg_type
{
	socket_arg_type( std::shared_ptr< socket_event > socket_event )
	: callback_arg_type{ callback_arg_type::event_type::socket_event }
	, socket_event_( std::move( socket_event ) )
	{ }

	std::weak_ptr< socket_event > socket_event_;
};

struct server_socket_arg_type
: callback_arg_type
{
	server_socket_arg_type( std::shared_ptr< server_socket > server_socket )
	: callback_arg_type{ callback_arg_type::event_type::server_socket }
	, server_socket_( std::move( server_socket ) )
	{ }

	std::weak_ptr< server_socket > server_socket_;
};

struct timer_arg_type
: callback_arg_type
{
	timer_arg_type( std::shared_ptr< timeout_event > timer_event )
	: callback_arg_type{ callback_arg_type::event_type::timer }
	, timer_event_( std::move( timer_event ) )
	{ }

	std::weak_ptr< timeout_event > timer_event_;
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

#ifndef QIO_USE_LIBEVENT_DNS
	q::queue_ptr dns_queue_;
	q::specific_execution_context_ptr< q::threadpool > dns_context_;
#endif

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

#endif // LIBQIO_INTERNAL_INTERNALS_HPP
