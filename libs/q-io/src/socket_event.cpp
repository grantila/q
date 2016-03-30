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

struct socket_event::pimpl
{
	socket_t fd_;

	::event* ev_read_;
	::event* ev_write_;

	std::atomic< bool > closed_;

	pimpl( socket_t sock )
	: fd_( sock )
	, ev_read_( nullptr )
	, ev_write_( nullptr )
	{ }
};

socket_event::socket_event( socket_t sock )
: pimpl_( make_unique< pimpl >( sock ) )
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

socket_t socket_event::socket( ) const
{
	return pimpl_->fd_;
}

void socket_event::sub_attach( const dispatcher_ptr& dispatcher ) noexcept
{
	auto& dispatcher_pimpl = get_dispatcher_pimpl( );
	auto self = socket_event_shared_from_this( );

	std::cout << "socket_event::sub_attach invoked" << std::endl;

	auto reader_ptr = new weak_socket_event_ptr{ self };
	auto writer_ptr = new weak_socket_event_ptr{ self };

	auto fn_read = [ ]( evutil_socket_t fd, short events, void* arg )
	-> void
	{
		std::cout << "running socket event callback [read]" << std::endl;

		auto socket = reinterpret_cast< weak_socket_event_ptr* >( arg );
		auto self = socket->lock( );

		if ( events & LIBQ_EV_CLOSE )
		{
			delete socket;
			return;
		}

		if ( !!self )
			self->on_event_read( );
	};

	auto fn_write = [ ]( evutil_socket_t fd, short events, void* arg )
	-> void
	{
		std::cout << "running socket event callback [write]" << std::endl;

		auto socket = reinterpret_cast< weak_socket_event_ptr* >( arg );
		auto self = socket->lock( );

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
/*
	DO THIS IN Q-IO SOCKETS:

	::event_add( pimpl_->ev_read_, nullptr );

	try_write( );
*/
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

} } // namespace io, namespace q
