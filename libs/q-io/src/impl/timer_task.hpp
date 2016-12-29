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

#ifndef LIBQIO_INTERNAL_IMPL_TIMER_TASK_HPP
#define LIBQIO_INTERNAL_IMPL_TIMER_TASK_HPP

#include <q-io/timer_task.hpp>

#include "handle.hpp"

namespace q { namespace io {

struct timer_task::pimpl
: handle
, std::enable_shared_from_this< timer_task::pimpl >
{
	typedef std::shared_ptr< timer_task::pimpl > data_ref_type;

	::uv_loop_t* loop_;
	::uv_timer_t timer_;

	std::shared_ptr< pimpl > cleanup_keepalive_ptr_;

	dispatcher_ptr dispatcher_;
	std::shared_ptr< q::task > task_;
	clock::duration duration_;
	clock::duration repeat_;

	pimpl( )
	: handle( reinterpret_cast< ::uv_handle_t* >( &timer_ ) )
	, loop_( nullptr )
	, duration_( clock::duration( 0 ) )
	, repeat_( clock::duration( 0 ) )
	{
		timer_.data = nullptr;
	}

	void
	attach_dispatcher( const dispatcher_ptr& dispatcher ) noexcept override;

	void close( ) override;

	void set_task( q::task task );
	void unset_task( );

	bool is_attached( ) const;
	void ensure_attached( ) const;
};

} } // namespace io, namespace q

#endif // LIBQIO_INTERNAL_IMPL_TIMER_TASK_HPP
