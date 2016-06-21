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

#include <q-io/socket_event.hpp>

#include "internals.hpp"

namespace q { namespace io {

#ifdef QIO_USE_LIBEVENT

socket_event::socket_event( socket_t sock )
: pimpl_( make_unique< pimpl >( sock ) )
{
}

socket_event::socket_event( std::unique_ptr< pimpl >&& pimpl )
: pimpl_( std::move( pimpl ) )
{
}

socket_event::~socket_event( )
{
	close_socket( );

	if ( pimpl_->ev_read_ )
		::event_free( pimpl_->ev_read_ );
	if ( pimpl_->ev_write_ )
		::event_free( pimpl_->ev_write_ );
}

void socket_event::detect_readability( )
{
	::event_add( pimpl_->ev_read_, nullptr );
}

void socket_event::detect_writability( )
{
	::event_add( pimpl_->ev_write_, nullptr );
}

void socket_event::trigger_read( )
{
	::event_active( pimpl_->ev_read_, EV_READ, 0 );
}

void socket_event::trigger_write( )
{
	::event_active( pimpl_->ev_write_, EV_WRITE, 0 );
}

socket_t socket_event::get_socket( ) const
{
	return pimpl_->fd_;
}

void socket_event::sub_attach( const dispatcher_ptr& dispatcher ) noexcept
{
	auto& dispatcher_pimpl = get_dispatcher_pimpl( );
	auto self = socket_event_shared_from_this( );

	auto reader_ptr = new socket_arg_type( self );
	auto writer_ptr = new socket_arg_type( self );

	event_callback_fn fn_read = [ ](
		evutil_socket_t fd, short events, void* arg
	)
	-> void
	{
		auto socket = reinterpret_cast< socket_arg_type* >( arg );
		auto self = socket->socket_event_.lock( );

		if ( events & LIBQ_EV_CLOSE )
		{
			delete socket;
			return;
		}

		if ( !!self )
			self->on_event_read( );
	};

	event_callback_fn fn_write = [ ](
		evutil_socket_t fd, short events, void* arg
	)
	-> void
	{
		auto socket = reinterpret_cast< socket_arg_type* >( arg );
		auto self = socket->socket_event_.lock( );

		if ( events & LIBQ_EV_CLOSE )
		{
			delete socket;
			return;
		}

		if ( !!self )
			self->on_event_write( );
	};

	auto event_base = dispatcher_pimpl.event_base;
	pimpl_->ev_read_ = ::event_new(
		event_base, pimpl_->fd_, EV_READ, fn_read, reader_ptr );
	pimpl_->ev_write_ = ::event_new(
		event_base, pimpl_->fd_, EV_WRITE, fn_write, writer_ptr );

	if ( !pimpl_->ev_write_ )
		delete writer_ptr;
	if ( !pimpl_->ev_read_ )
		delete reader_ptr;

	on_attached( dispatcher );
}

void socket_event::close_socket( )
{
	auto was_closed = pimpl_->closed_.exchange( true );

	if ( was_closed )
		return;

	if ( pimpl_->fd_ != invalid_socket::value )
		EVUTIL_CLOSESOCKET( pimpl_->fd_ );

	if ( pimpl_->ev_read_ != nullptr )
	{
		::event_del( pimpl_->ev_read_ );
		::event_active( pimpl_->ev_read_, LIBQ_EV_CLOSE, 0 );
	}

	if ( pimpl_->ev_write_ != nullptr )
	{
		::event_del( pimpl_->ev_write_ );
		::event_active( pimpl_->ev_write_, LIBQ_EV_CLOSE, 0 );
	}
}

#endif

} } // namespace io, namespace q
