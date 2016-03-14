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

#include <q-io/timeout_event.hpp>

#include "internals.hpp"

#include <event2/event.h>

namespace q { namespace io {

struct timeout_event::pimpl
{
	q::task task_;
	clock::duration duration_;
};

timeout_event::timeout_event( q::task task, clock::duration duration )
: pimpl_( new pimpl{ task, duration } )
{ }

std::shared_ptr< timeout_event >
timeout_event::construct( q::task task, clock::duration duration )
{
	return q::make_shared_using_constructor<
		timeout_event
	>( std::move( task ), std::move( duration ) );
}

void timeout_event::sub_attach( const dispatcher_ptr& dispatcher ) noexcept
{
	std::cout << "timeout_event::sub_attach invoked" << std::endl;
	auto self = new std::shared_ptr< timeout_event >( shared_from_this( ) );

	auto fn = [ ]( evutil_socket_t fd, short events, void* arg ) -> void
	{
		std::cout << "running event callback" << std::endl;
		auto self = reinterpret_cast< std::shared_ptr< timeout_event >* >( arg );
		(*self)->on_event_timeout( );
		delete self;
	};

	auto event_base = get_dispatcher_pimpl( ).event_base;
	::event* ev = ::event_new( event_base, -1, EV_TIMEOUT, fn, self );

	struct timeval timeout = clock::to_timeval( pimpl_->duration_ );
	std::cout << timeout.tv_sec << " " << timeout.tv_usec << std::endl;
	std::cout << event_initialized( ev ) << std::endl;
	::event_add( ev, &timeout );
	std::cout << "C" << std::endl;

}

void timeout_event::on_event_timeout( ) noexcept
{
	Q_ENSURE_NOEXCEPT( pimpl_->task_ );
}

} } // namespace io, namespace q
