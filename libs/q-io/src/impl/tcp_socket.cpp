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

#include "tcp_socket.hpp"
#include "dispatcher.hpp"

namespace q { namespace io {

typedef tcp_socket::pimpl::data_ref_type inner_ref_type;

namespace {

void closer( ::uv_handle_t* handle )
{
	auto socket = reinterpret_cast< ::uv_tcp_t* >( handle );
	auto ref = reinterpret_cast< inner_ref_type* >( socket->data );
	socket->data = nullptr;

	if ( ref )
		delete ref;
};

} // anonymous namespace

std::shared_ptr< tcp_socket::pimpl > tcp_socket::pimpl::construct( )
{
	auto pimpl = q::make_shared_using_constructor< tcp_socket::pimpl >( );
	pimpl->self_ = pimpl;
	pimpl->socket_.data = nullptr;
	return pimpl;
}

void
tcp_socket::pimpl::attach_dispatcher( const dispatcher_ptr& dispatcher )
noexcept
{
	auto& dispatcher_pimpl = *dispatcher->pimpl_;

	auto u_ref = q::make_unique< data_ref_type >( shared_from_this( ) );
	auto ref = u_ref.release( );

	socket_.data = reinterpret_cast< void* >( ref );

	// TODO: Reconsider these
	std::size_t backlog_in = 6;
	std::size_t backlog_out = 10;

	auto queue = dispatcher_pimpl.user_queue;

	channel< q::byte_block > channel_in( queue, backlog_in );
	channel< q::byte_block > channel_out( queue, backlog_out );

	readable_in_ = std::make_shared< q::readable< q::byte_block > >(
		channel_in.get_readable( ) );
	writable_in_ = std::make_shared< q::writable< q::byte_block > >(
		channel_in.get_writable( ) );
	readable_out_ = std::make_shared< q::readable< q::byte_block > >(
		channel_out.get_readable( ) );
	writable_out_ = std::make_shared< q::writable< q::byte_block > >(
		channel_out.get_writable( ) );

	start_read( );
	begin_write( );
}

void tcp_socket::pimpl::close( expect< void > status )
{
	if ( closed_.exchange( true ) )
		return;

	auto writable_in = std::atomic_load( &writable_in_ );
	auto readable_out = std::atomic_load( &readable_out_ );

	if ( !socket_.data )
	{
		auto unique_ref =
			q::make_unique< inner_ref_type >( shared_from_this( ) );
		socket_.data = unique_ref.release( );
	}

	if ( !!writable_in )
	{
		writable_in->unset_resume_notification( );
		if ( status.has_exception( ) )
			writable_in->close( status.exception( ) );
		else
			writable_in->close( );
	}

	if ( !!readable_out )
	{
		if ( status.has_exception( ) )
			readable_out->close( status.exception( ) );
		else
			readable_out->close( );
	}

	stop_read( );

	writable_in_.reset( );
	readable_out_.reset( );

	auto handle = reinterpret_cast< ::uv_handle_t* >( &socket_ );
	::uv_close( handle, closer );
}

void tcp_socket::pimpl::start_read( )
{
	auto stream = reinterpret_cast< ::uv_stream_t* >( &socket_ );

	auto alloc_cb = [ ](
		::uv_handle_t* handle,
		::size_t suggested_size,
		::uv_buf_t* buf
	)
	{
		buf->base = new char[ suggested_size ];
		buf->len = suggested_size;
	};

	auto read_cb = [ ](
		::uv_stream_t* stream,
		::ssize_t nread,
		const uv_buf_t* buf
	)
	{
		auto pimpl =
			*reinterpret_cast< inner_ref_type* >( stream->data );

		if ( nread > 0 )
		{
			// Data

			byte_block block(
				nread,
				reinterpret_cast< std::uint8_t* >( buf->base )
			);

			if ( !pimpl->writable_in_->send( std::move( block ) ) )
				pimpl->stop_read( false );
			else if ( !pimpl->writable_in_->should_send( ) )
				pimpl->stop_read( true );

			return;
		}

		// If a buffer has been allocated but we can't read (because
		// closed connection or error), we need to clean it up.
		if ( buf->base )
			delete[ ] buf->base;

		if ( nread == UV_EOF )
		{
			// Close without error

			pimpl->close( );
		}
		else if ( nread < 0 )
		{
			// Error

			auto errno_ = uv_error_to_errno( nread );
			auto err = get_exception_by_errno( errno_ );
			pimpl->close( err );
		}
	};

	::uv_read_start( stream, alloc_cb, read_cb );
}

void tcp_socket::pimpl::stop_read( bool reschedule )
{
	auto stream = reinterpret_cast< ::uv_stream_t* >( &socket_ );

	::uv_read_stop( stream );

	if ( !reschedule )
		return;

	auto pimpl = self_.lock( );

	writable_in_->set_resume_notification(
		[ pimpl ]( )
		{
			pimpl->start_read( );
		},
		true
	);
}

void tcp_socket::pimpl::begin_write( )
{
	auto pimpl = self_.lock( );
	auto stream = reinterpret_cast< ::uv_stream_t* >( &socket_ );

	::uv_write_cb read_again = [ ]( ::uv_write_t* req, int status )
	{
		auto pimpl_ptr =
			reinterpret_cast< write_req_self_ptr* >( req->data );
		auto pimpl = *pimpl_ptr;
		delete pimpl_ptr;

		bool should_write_more = false;

		{
			Q_AUTO_UNIQUE_LOCK( pimpl->mut_ );

			auto iter = std::find_if(
				pimpl->write_reqs_.begin( ),
				pimpl->write_reqs_.end( ),
				[ req ]( const write_info& item )
				{
					return req == item.req_.get( );
				}
			);

			if ( iter == pimpl->write_reqs_.end( ) )
			{
				// This really cannot happen
				std::cerr
					<< "TODO: Remove this check"
					<< std::endl;
				pimpl->close( );
			}

			auto buf_size = iter->buf_len_;

			// This implicitly frees req!
			pimpl->write_reqs_.erase( iter );

			auto size_before = pimpl->cached_bytes_;
			pimpl->cached_bytes_ -= buf_size;

			should_write_more = size_before >= pimpl->cache_size &&
				pimpl->cached_bytes_ < pimpl->cache_size;
		}

		if ( status == 0 )
		{
			// Success, write more if we should
			if ( should_write_more )
				pimpl->begin_write( );
		}
		else
		{
			// Failure, propagate upstream and close
			// TODO: Potentially check for proper error and
			//       propagate it. Right now we just close "nicely".
			pimpl->close( );
		}
	};

	if ( !readable_out_ )
		// Already closed
		return;

	readable_out_->receive(
		[ pimpl, stream, read_again ]( byte_block&& block ) mutable
		{
			if ( pimpl->closed_ )
				return;

			auto req = q::make_unique< ::uv_write_t >( );
			req->data = new write_req_self_ptr( pimpl );
			// `data` is written here and read by another thread.
			// When running with race condition analysis, this will
			// look like a race condition unless we kill_dependency
			std::kill_dependency( req->data );

			::uv_buf_t buf{
				.base = reinterpret_cast< char* >(
					const_cast< std::uint8_t* >(
						block.data( ) ) ),
				.len = block.size( )
			};

			bool should_read_more = true;

			{
				Q_AUTO_UNIQUE_LOCK( pimpl->mut_ );

				pimpl->cached_bytes_ += buf.len;
				if ( pimpl->cached_bytes_ >= pimpl->cache_size )
					should_read_more = false;

				::uv_write_t* write_req = req.get( );

				pimpl->write_reqs_.push_back( write_info{
					std::move( req ),
					std::move( block ),
					buf.len
				} );

				::uv_write(
					write_req,
					stream,
					&buf,
					1,
					read_again );
			}

			if ( should_read_more )
				pimpl->begin_write( );
		},
		[ pimpl ]( ) mutable
		{
			pimpl->close( );
		}
	)
	.fail( [ pimpl ]( std::exception_ptr err ) mutable
	-> bool
	{
		// Internal read error. Not much we can do, except close the
		// connection.
		// TODO: Consider allowing a callback or custom handler for
		// these channel errors (e.g. for logging).
		pimpl->close( );
		return false;
	} );
}

} } // namespace io, namespace q
