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

#include "internals.hpp"

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
{
	event::set_pimpl( &pimpl_->event_ );

	pimpl_->timer_.data = nullptr;
}

timer_task::~timer_task( )
{
	::uv_close_cb closer = [ ]( ::uv_handle_t* handle )
	{
		auto timer = reinterpret_cast< ::uv_timer_t* >( handle );
		auto ref = reinterpret_cast< pimpl* >( timer->data );

		if ( !ref )
			// This should happen. It means we have initialized
			// this timer half-way somehow.
			// TODO: Assert we don't end up here
			return;

		// Keeps the pimpl alive until the end of this scope
		auto keep_alive = ref->cleanup_keepalive_ptr_;

		ref->cleanup_keepalive_ptr_.reset( );
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
	ensure_attached( );

	std::atomic_store(
		&pimpl_->task_,
		std::make_shared< q::task >( std::move( task ) )
	);
}

void timer_task::unset_task( )
{
	ensure_attached( );

	std::atomic_store( &pimpl_->task_, std::shared_ptr< q::task >( ) );
}

void timer_task::start_timeout(
	clock::duration duration, clock::duration repeat
)
{
	ensure_attached( );

	pimpl_->duration_ = duration;
	pimpl_->repeat_ = repeat;

	auto _pimpl = pimpl_;

	auto starter = [ _pimpl ]( )
	{
		::uv_timer_t* timer = &_pimpl->timer_;
		/* ret */::uv_timer_stop( timer );

		uv_timer_cb timer_callback = [ ]( ::uv_timer_t* timer )
		{
			auto pimpl_ = reinterpret_cast< pimpl* >( timer->data );

			auto task = std::atomic_load( &pimpl_->task_ );
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
	ensure_attached( );

	auto _pimpl = pimpl_;

	auto stopper = [ _pimpl ]( )
	{
		::uv_timer_t* timer = &_pimpl->timer_;
		/* ret */::uv_timer_stop( timer );
	};

	pimpl_->dispatcher_->get_queue( )->push( stopper );
}

void timer_task::sub_attach( const dispatcher_ptr& dispatcher ) noexcept
{
	if ( is_attached( ) )
		Q_THROW( event_error( ), "Already attached" ); // Better error

	pimpl_->dispatcher_ = dispatcher;
	pimpl_->loop_ = &pimpl_->dispatcher_->pimpl_->uv_loop;

	if ( ::uv_timer_init( pimpl_->loop_, &pimpl_->timer_ ) )
		// TODO: Better error, well described and thought through logic
		Q_THROW( event_error( ) );

	pimpl_->timer_.data = &*pimpl_;
}

bool timer_task::is_attached( ) const
{
	return pimpl_->loop_;
}

void timer_task::ensure_attached( ) const
{
	if ( !is_attached( ) )
		Q_THROW( event_error( ) ); // TODO: Better error
}

} } // namespace io, namespace q
