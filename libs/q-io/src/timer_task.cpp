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

#include <q-io/timer_task.hpp>

#include <q/types.hpp>

#include "impl/timer_task.hpp"

namespace q { namespace io {

namespace {

template< typename Duration >
typename std::decay< Duration >::type::rep to_millis( Duration&& duration )
{
	typedef std::chrono::milliseconds millis;
	auto dur_millis = std::chrono::duration_cast< millis >( duration );
	return dur_millis.count( );
}

} // anonymous namespace

timer_task::timer_task( )
: pimpl_( std::make_shared< pimpl >( ) )
{ }

timer_task::~timer_task( )
{
	::uv_close_cb closer = [ ]( ::uv_handle_t* handle )
	{
		auto timer = reinterpret_cast< ::uv_timer_t* >( handle );
		auto ref = reinterpret_cast< pimpl::data_ref_type* >(
			timer->data );
		timer->data = nullptr;

		if ( ref )
			delete ref;

		auto pimpl = *ref;

		if ( !pimpl )
			// This should happen. It means we have initialized
			// this timer half-way somehow.
			// TODO: Assert we don't end up here
			return;

		// Keeps the pimpl alive until the end of this scope
		auto keep_alive = pimpl->cleanup_keepalive_ptr_;

		pimpl->cleanup_keepalive_ptr_.reset( );
	};

	auto handle = reinterpret_cast< ::uv_handle_t* >( &pimpl_->timer_ );

	if ( pimpl_->loop_ )
	{
		// This timer has been initialized, and needs to be closed
		pimpl_->cleanup_keepalive_ptr_ = pimpl_;
		::uv_close( handle, closer );
	}
}

void timer_task::set_task( q::task task )
{
	pimpl_->set_task( std::move( task ) );
}

void timer_task::unset_task( )
{
	pimpl_->unset_task( );
}

void timer_task::start_timeout(
	clock::duration duration, clock::duration repeat
)
{
	pimpl_->ensure_attached( );

	pimpl_->duration_ = duration;
	pimpl_->repeat_ = repeat;

	auto _pimpl = pimpl_;

	auto starter = [ _pimpl ]( )
	{
		::uv_timer_t* timer = &_pimpl->timer_;
		/* ret */::uv_timer_stop( timer );

		uv_timer_cb timer_callback = [ ]( ::uv_timer_t* timer )
		{
			auto ref = reinterpret_cast< pimpl::data_ref_type* >(
				timer->data );
			auto pimpl = *ref;

			auto task = std::atomic_load( &pimpl->task_ );
			if ( task && *task )
				( *task )( );
		};

		auto dur_millis = to_millis( _pimpl->duration_ );
		auto rep_millis = to_millis( _pimpl->repeat_ );

		/* ret */::uv_timer_start(
			timer, timer_callback, dur_millis, rep_millis );
	};

	pimpl_->dispatcher_->get_queue( )->push( starter );
}

void timer_task::stop( )
{
	pimpl_->ensure_attached( );

	auto _pimpl = pimpl_;

	auto stopper = [ _pimpl ]( )
	{
		::uv_timer_t* timer = &_pimpl->timer_;
		/* ret */::uv_timer_stop( timer );
	};

	pimpl_->dispatcher_->get_queue( )->push( stopper );
}

} } // namespace io, namespace q
