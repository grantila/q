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

#include <q-io/socket.hpp>

#include <q/scope.hpp>

#include "internals.hpp"

#include <event2/event.h>

#ifdef LIBQ_ON_WINDOWS
#	include <winsock2.h>
#	define ioctl ioctlsocket
#else
#	include <sys/ioctl.h>
#endif

namespace q { namespace io {

//
// TODO: This implementation currently has an implicit circular dependency
//       between the socket and its two channels (through lambda-captured
//       shared_ptr's). This needs to be fixed if the user chooses to not refer
//       to any of these instances and just leave them. It should close the
//       socket and clean up.
//

struct socket::pimpl
{
	q::channel_ptr< q::byte_block > channel_in_;
	q::channel_ptr< q::byte_block > channel_out_;
	q::weak_channel_ptr< q::byte_block > weak_channel_in_;
	q::weak_channel_ptr< q::byte_block > weak_channel_out_;
	native_socket socket_;

	::event* ev_read_;
	::event* ev_write_;

	std::atomic< bool > can_read_;
	std::atomic< bool > can_write_;
	q::byte_block out_buffer_;

	std::atomic< bool > closed_;
};

socket::socket( const native_socket& s )
: pimpl_( new pimpl {
	nullptr,
	nullptr,
	q::channel_ptr< q::byte_block >( nullptr ),
	q::channel_ptr< q::byte_block >( nullptr ),
	s,
	nullptr,
	nullptr
} )
{
	pimpl_->can_read_ = false;
	pimpl_->can_write_ = true;
	pimpl_->closed_ = false;
}

socket::~socket( )
{
	close_socket( );

	if ( pimpl_->ev_read_ )
		::event_free( pimpl_->ev_read_ );
	if ( pimpl_->ev_write_ )
		::event_free( pimpl_->ev_write_ );
}

socket_ptr socket::construct( const native_socket& s )
{
	auto sock = q::make_shared< socket >( s );

	return sock;
}

q::readable_ptr< q::byte_block > socket::in( )
{
	return _in( );
}

q::writable_ptr< q::byte_block > socket::out( )
{
	return _out( );
}

void socket::detach( )
{
	// Only use weak_ptr's to the channels, and let them own this socket.

	auto in = std::atomic_load( &pimpl_->channel_in_ );
	auto out = std::atomic_load( &pimpl_->channel_out_ );

	if ( !in )
		// Already detached (probably)
		return;

	in->add_scope_until_closed( ::q::make_scope( shared_from_this( ) ) );
	out->add_scope_until_closed( ::q::make_scope( shared_from_this( ) ) );

	in.reset( );
	out.reset( );

	std::atomic_store( &pimpl_->channel_in_, in );
	std::atomic_store( &pimpl_->channel_out_, out );
}

q::channel_ptr< q::byte_block > socket::_in( )
{
	auto in = std::atomic_load( &pimpl_->channel_in_ );
	if ( !in )
		in = pimpl_->weak_channel_in_.lock( );
	return in;
}

q::channel_ptr< q::byte_block > socket::_out( )
{
	auto out = std::atomic_load( &pimpl_->channel_out_ );
	if ( !out )
		out = pimpl_->weak_channel_out_.lock( );
	return out;
}

void socket::sub_attach( const dispatcher_ptr& dispatcher ) noexcept
{
	auto& dispatcher_pimpl = get_dispatcher_pimpl( );

	pimpl_->channel_in_ = std::make_shared< q::channel< q::byte_block > >(
		dispatcher_pimpl.user_queue, 10 ); // 10 backlog items
	pimpl_->channel_out_ = std::make_shared< q::channel< q::byte_block > >(
		dispatcher_pimpl.user_queue, 10 ); // 10 backlog items

	pimpl_->weak_channel_in_ = pimpl_->channel_in_;
	pimpl_->weak_channel_out_ = pimpl_->channel_out_;

	auto self = shared_from_this( );
	auto weak_self = weak_socket_ptr{ self };

	pimpl_->channel_in_->set_resume_notification( [ weak_self ]( )
	{
		std::cout << "WILL RETRY" << std::endl;
		auto self = weak_self.lock( );
		if ( !self )
			return; // TODO: Analyze when this can happen
		::event_active( self->pimpl_->ev_read_, EV_READ, 0 );
	} );

	std::cout << "client_socket::sub_attach invoked" << std::endl;

	auto reader_ptr = new weak_socket_ptr{ self };
	auto writer_ptr = new weak_socket_ptr{ self };

	auto fn_read = [ ]( evutil_socket_t fd, short events, void* arg )
	-> void
	{
		std::cout << "running socket event callback [read]" << std::endl;

		auto socket = reinterpret_cast< weak_socket_ptr* >( arg );
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

		auto socket = reinterpret_cast< weak_socket_ptr* >( arg );
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
		event_base, pimpl_->socket_.fd, EV_READ, fn_read, reader_ptr );
	pimpl_->ev_write_ = ::event_new(
		event_base, pimpl_->socket_.fd, EV_WRITE, fn_write, writer_ptr );

	if ( !pimpl_->ev_write_ )
		delete writer_ptr;
	if ( !pimpl_->ev_read_ )
		delete reader_ptr;

	::event_add( pimpl_->ev_read_, nullptr );

	try_write( );
}

// TODO: Analyse exceptions
void socket::on_event_read( ) noexcept
{
	std::cout << "ON_EVENT_READ" << std::endl;

	// Due to potential edge-triggered IO, we'll read everything we
	// can now, and if necessary mark this socket as still readable
	// until we get an EAGAIN.
	std::cout << "CAN READ" << std::endl;

	bool done = false;

	auto in = _in( );
	if ( !in )
	{
		pimpl_->can_read_ = false;
		close_socket( );
		return;
	}

	while ( in->should_send( ) )
	{
#ifdef LIBQ_ON_WINDOWS
		unsigned long nbytes;
#else
		int nbytes;
#endif
		std::size_t block_size = 8 * 1024;
		if ( !::ioctl( pimpl_->socket_.fd, FIONREAD, &nbytes ) )
			block_size = std::max( nbytes, 1 );

		std::cout << "CAN READ " << block_size << " BYTES" << std::endl;

		q::byte_block block( block_size );

		auto read_bytes = ::read(
			pimpl_->socket_.fd,
			block.data( ),
			block.size( ) );

		if ( read_bytes > 0 )
		{
			block.resize( read_bytes );
			pimpl_->channel_in_->send( std::move( block ) );
		}

		else if ( read_bytes == 0 )
		{
			done = true;
			close_socket( );
		}

		else if ( read_bytes == -1 )
		{
			if ( errno == EAGAIN )
			{
				// We've read all we can
				done = true;
				break;
			}
			else
			{
				// TODO: Handle error
			}
		}
	}

	if ( done )
	{
		// Finished reading, we need to ensure we listen to
		// more reads, by adding the event to the loop.
		::event_add( pimpl_->ev_read_, nullptr );
		pimpl_->can_read_ = false;
	}
	else
		// There is more to read, we'll signal this internally
		// through the channels.
		pimpl_->can_read_ = true;
}

// TODO: Analyse exceptions
void socket::on_event_write( ) noexcept
{
	std::cout << "ON_EVENT_WRITE" << std::endl;

	// Write as much as possible.
	std::cout << "CAN WRITE" << std::endl;

	try_write( );
}

void socket::try_write( )
{
	auto self = shared_from_this( );

	auto try_write_block = [ self, this ]( q::byte_block&& block )
	-> bool // True means we still have things to write - save buffer
	{
		if ( block.size( ) == 0 )
			// Empty block? Let's try the next block.
			try_write( );

		auto written_bytes = ::write(
			pimpl_->socket_.fd,
			block.data( ),
			block.size( ) );

		std::cout << "WROTE " << written_bytes << " BYTES. fd=" << pimpl_->socket_.fd << std::endl;

		if ( written_bytes < 0 )
		{
			if ( errno == EAGAIN )
				::event_add( pimpl_->ev_write_, nullptr );
			else
				// TODO: Handle error
				std::cout << "ERROR " << strerror( errno ) << std::endl;
		}

		else if ( written_bytes == block.size( ) )
		{
			// Wrote the whole chunk, try to write more
			try_write( );
		}

		else
		{
			// We wrote something, but not everything. Advance
			// buffer, save it for next iteration and await
			// libevent to tell us when we can write again.
			block.advance(
				static_cast< std::size_t >( written_bytes ) );

			pimpl_->out_buffer_ = std::move( block );

			::event_add( pimpl_->ev_write_, nullptr );

			return true;
		}

		return false;
	};

	if ( pimpl_->out_buffer_.size( ) > 0 )
	{
		if ( try_write_block( std::move( pimpl_->out_buffer_ ) ) )
			return;
	}

	auto out = _out( );

	if ( !out )
	{
		close_socket( );
		return;
	}

	out->receive( )
	.then( [ self, this, try_write_block ]( q::byte_block&& block )
	{
		std::cout
			<< "RECEIVE GOT "
			<< block.size( )
			<< " BYTES:"
			<< block.to_string( )
			<< std::endl;
		try_write_block( std::move( block ) );
	} )
	.fail( [ ]( const q::channel_closed_exception& )
	{
		; // TODO: Do something useful.
	} )
	.fail( [ ]( std::exception_ptr e )
	{
		; // TODO: Do something. Close, set error, ...
	} );
}

void socket::close_socket( )
{
	auto was_closed = pimpl_->closed_.exchange( true );

	if ( was_closed )
		return;

	EVUTIL_CLOSESOCKET( pimpl_->socket_.fd );

	::event_del( pimpl_->ev_read_ );
	::event_del( pimpl_->ev_write_ );

	::event_active( pimpl_->ev_read_, LIBQ_EV_CLOSE, 0 );
	::event_active( pimpl_->ev_write_, LIBQ_EV_CLOSE, 0 );

	auto in = _in( );
	auto out = _out( );

	if ( !!in )
	{
		in->set_resume_notification( nullptr );
		in->close( );
	}

	if ( !!out )
		out->close( );
}

} } // namespace io, namespace q
