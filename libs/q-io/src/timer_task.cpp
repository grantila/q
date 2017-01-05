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
#include "impl/dispatcher.hpp"

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
{ }

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

	std::weak_ptr< pimpl > weak_pimpl = pimpl_;

	auto starter = [ weak_pimpl, duration, repeat ]( )
	{
		auto pimpl = weak_pimpl.lock( );
		if ( !pimpl )
			return;

		pimpl->duration_ = duration;
		pimpl->repeat_ = repeat;

		::uv_timer_t* timer = &pimpl->timer_;
		/* ret */::uv_timer_stop( timer );

		uv_timer_cb timer_callback = [ ]( ::uv_timer_t* timer )
		{
			auto pimpl = get_pimpl< timer_task::pimpl >( timer );

			auto task = std::atomic_load( &pimpl->task_ );
			if ( task && *task )
				( *task )( );
		};

		auto dur_millis = to_millis( pimpl->duration_ );
		auto rep_millis = to_millis( pimpl->repeat_ );

		/* ret */::uv_timer_start(
			timer, timer_callback, dur_millis, rep_millis );
	};

	pimpl_->dispatcher_->internal_queue_->push( std::move( starter ) );
}

void timer_task::stop( )
{
	pimpl_->ensure_attached( );

	std::weak_ptr< pimpl > weak_pimpl = pimpl_;

	auto stopper = [ weak_pimpl ]( )
	{
		auto pimpl = weak_pimpl.lock( );
		if ( !pimpl )
			return;

		::uv_timer_t* timer = &pimpl->timer_;
		/* ret */::uv_timer_stop( timer );
	};

	pimpl_->dispatcher_->internal_queue_->push( std::move( stopper ) );
}

} } // namespace io, namespace q
