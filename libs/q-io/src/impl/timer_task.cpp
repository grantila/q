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

#include "timer_task.hpp"
#include "dispatcher.hpp"

namespace q { namespace io {

void
timer_task::pimpl::i_attach_dispatcher( const dispatcher_pimpl_ptr& dispatcher )
noexcept
{
	if ( is_attached( ) )
		Q_THROW( event_error( ), "Already attached" ); // Better error

	dispatcher_ = dispatcher;
	loop_ = &dispatcher_->uv_loop;

	if ( ::uv_timer_init( loop_, &timer_ ) )
		// TODO: Better error, well described and thought through logic
		Q_THROW( event_error( ) );

	keep_alive_ = shared_from_this( );
}

void timer_task::pimpl::i_close( q::expect< void > exp )
{
	::uv_timer_stop( &timer_ );

	i_close_handle( );
}

void timer_task::pimpl::set_task( q::task task )
{
	ensure_attached( );

	std::atomic_store(
		&task_,
		std::make_shared< q::task >( std::move( task ) )
	);
}

void timer_task::pimpl::unset_task( )
{
	ensure_attached( );

	std::atomic_store( &task_, std::shared_ptr< q::task >( ) );
}

bool timer_task::pimpl::is_attached( ) const
{
	return loop_;
}

void timer_task::pimpl::ensure_attached( ) const
{
	if ( !is_attached( ) )
		Q_THROW( event_error( ) ); // TODO: Better error
}


} } // namespace io, namespace q
