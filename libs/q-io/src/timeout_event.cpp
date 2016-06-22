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

namespace q { namespace io {

timeout_event::timeout_event( )
: pimpl_( ::q::make_unique< pimpl >( ) )
{
//	pimpl_->event_ = nullptr;
	pimpl_->timer_.data = nullptr;
}

timeout_event::~timeout_event( )
{
	if ( pimpl_->event_ )
	{
		::event_del( pimpl_->event_ );
		::event_active( pimpl_->event_, LIBQ_EV_CLOSE, 0 );

		::event_free( pimpl_->event_ );
	}
}

void timeout_event::set_timeout_now( )
{
	remove_timeout( );
	::event_active( pimpl_->event_, EV_TIMEOUT, 0 );
}

void timeout_event::set_timeout( clock::duration duration )
{
	struct timeval timeout = clock::to_timeval( duration );

	::event_add( pimpl_->event_, &timeout );
}

void timeout_event::remove_timeout( )
{
	::event_del( pimpl_->event_ );
}

void timeout_event::sub_attach( const dispatcher_ptr& dispatcher ) noexcept
{
	auto& dispatcher_pimpl = *dispatcher->pimpl_;

	pimpl_->loop_ = &dispatcher_pimpl.uv_loop;

	if ( ::uv_timer_init( pimpl_->loop_, &pimpl_->timer_ ) )
		Q_THROW( event_error( ) );

	;

	auto self = new timer_arg_type( shared_from_this( ) );

	event_callback_fn fn = [ ](
		evutil_socket_t fd, short events, void* arg
	)
	-> void
	{
		auto weak_self = reinterpret_cast< timer_arg_type* >( arg );

		auto self = weak_self->timer_event_.lock( );

		if ( ( events & EV_TIMEOUT ) && !!self )
			self->on_event_timeout( );

		if ( events & LIBQ_EV_CLOSE )
			delete weak_self;
	};

#ifdef QIO_USE_LIBEVENT
	auto event_base = get_dispatcher_pimpl( ).event_base;
	pimpl_->event_ = ::event_new( event_base, -1, EV_TIMEOUT, fn, self );
#else
	// TODO: Implement for libuv
#endif

	on_initialized( );
}


std::shared_ptr< timeout_task >
timeout_task::construct( q::task task, clock::duration duration )
{
	return ::q::make_shared_using_constructor< timeout_task >(
		std::move( task ), std::move( duration ) );
}

timeout_task::timeout_task( q::task task, clock::duration duration )
: task_( std::move( task ) )
, duration_( std::move( duration ) )
{ }

timeout_task::~timeout_task( )
{ }

void timeout_task::on_event_timeout( ) noexcept
{
	auto task = std::move( task_ );
	self_ptr_.reset( );
	Q_ENSURE_NOEXCEPT( task );
}

void timeout_task::on_initialized( ) noexcept
{
	self_ptr_ = std::static_pointer_cast< timeout_task >(
		shared_from_this( ) );

	set_timeout( duration_ );
}

} } // namespace io, namespace q
