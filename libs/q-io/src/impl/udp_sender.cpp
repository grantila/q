/*
 * Copyright 2017 Gustaf Räntilä
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

#include "udp_sender.hpp"
#include "dispatcher.hpp"

namespace q { namespace io {

namespace {

static void closer( ::uv_handle_t* handle )
{
	auto socket = reinterpret_cast< ::uv_udp_t* >( handle );
	auto pimpl = reinterpret_cast< udp_sender::pimpl::data_ref_type >(
		socket->data );

	pimpl->keep_alive_.reset( );
};

} // anonymous namespace

std::shared_ptr< udp_sender::pimpl >
udp_sender::pimpl::construct(
	queue_ptr internal_queue,
	ip_address addr,
	std::uint16_t port,
	udp_send_options options
)
{
	auto pimpl = q::make_shared_using_constructor< udp_sender::pimpl >( );

	pimpl->keep_alive_ = pimpl;

	std::size_t bl = std::numeric_limits< std::size_t >::max( );
	if ( options.has< q::backlog >( ) )
	{
		auto backlog = options.get< q::backlog >( );
		bl = std::max< std::size_t >( backlog.get( ), 1 );

		pimpl->is_infinite_ = backlog.is_infinity( );
	}
	else
		pimpl->is_infinite_ = true;

	pimpl->sockaddr_ = addr.get_sockaddr( port );

	pimpl->construction_options_ = q::make_unique< udp_send_options >(
		std::move( options ) );

	q::channel< byte_block > ch( internal_queue, bl, bl - 1 );

	pimpl->readable_out_ = std::make_shared< q::readable< byte_block > >(
		ch.get_readable( ) );
	pimpl->writable_out_ = std::make_shared< q::writable< byte_block > >(
		ch.get_writable( ) );

	return pimpl;
}

// TODO: make this function "i_attach_dispatcher" and ensure outside calls
//       schedule this on the internal thread.
void
udp_sender::pimpl::attach_dispatcher( const dispatcher_ptr& dispatcher )
noexcept
{
	keep_alive_ = shared_from_this( );

	udp_.data = reinterpret_cast< void* >( this );

	::uv_udp_init( &dispatcher->pimpl_->uv_loop, &udp_ );

	auto bind_to = construction_options_->
		template get< ip_address_and_port >(
			ip_address_and_port( ip_address( "0.0.0.0" ), 0 ) );
	auto bind_flags = construction_options_->template get< udp_bind >( );

	auto sockaddr = bind_to.get_sockaddr( );

	unsigned int flags = 0
		| ( Q_ENUM_HAS( bind_flags, udp_bind::ip_v6_only )
			? UV_UDP_IPV6ONLY : 0 )
		| ( Q_ENUM_HAS( bind_flags, udp_bind::reuseaddr )
			? UV_UDP_REUSEADDR : 0 );

	::uv_udp_bind( &udp_, sockaddr.get( ), flags );

	construction_options_.reset( );

	read_write_one( );
}

// TODO: make this function "i_close" and ensure outside calls (in public class
//       destructors e.g.) schedule this on the internal thread.
void udp_sender::pimpl::close( expect< void > status )
{
	if ( closed_.exchange( true ) )
		return;

	write_reqs_.clear( );

	auto readable_out = std::atomic_load( &readable_out_ );

	if ( !!readable_out )
	{
		if ( status.has_exception( ) )
			readable_out->close( status.exception( ) );
		else
			readable_out->close( );
	}

	readable_out.reset( );

	auto handle = reinterpret_cast< ::uv_handle_t* >( &udp_ );
	::uv_close( handle, closer );
}

void udp_sender::pimpl::send_block( ::q::byte_block block )
{
	::uv_udp_send_cb send_cb = [ ]( ::uv_udp_send_t* req, int status )
	{
		std::cout << "SEND STATUS " << status << std::endl;

		write_info* info = reinterpret_cast< write_info* >( req->data );

		auto pimpl = info->keep_alive_;

		auto iter = std::find_if(
			pimpl->write_reqs_.begin( ),
			pimpl->write_reqs_.end( ),
			[ info ]( const std::unique_ptr< write_info >& other )
			{
				return info->req_.data == other->req_.data;
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

		pimpl->write_reqs_.erase( iter );

		pimpl->read_write_one( );
	};

	::uv_buf_t buf{
		.base = reinterpret_cast< char* >(
			const_cast< std::uint8_t* >( block.data( ) ) ),
		.len = block.size( )
	};

	auto info = q::make_unique< write_info >( );
	info->keep_alive_ = shared_from_this( );
	info->req_.data = info.get( ); // Mark
	info->block_ = std::move( block );
	info->buf_len_ = buf.len;

	::uv_udp_send_t* send_req = &info->req_;

	write_reqs_.push_back( std::move( info ) );

	::uv_udp_send( send_req, &udp_, &buf, 1, sockaddr_.get( ), send_cb );
}

void udp_sender::pimpl::read_write_one( )
{
	auto pimpl = shared_from_this( );
	auto readable_out = std::atomic_load( &readable_out_ );

	readable_out->read( [ pimpl ]( ::q::byte_block&& block ) mutable
	{
		pimpl->send_block( std::move( block ) );
	}, [ pimpl ]( ) mutable
	{
		pimpl->close( );
	} )
	.strip( )
	.fail( [ pimpl ]( std::exception_ptr err )
	{
		pimpl->close( err );
	} );
}

void udp_sender::pimpl::detach( )
{
	auto self = shared_from_this( );

	auto scope = [ self ]( ) mutable
	{
		auto w = std::make_shared< writable< ::q::byte_block > >( );
		auto r = std::make_shared< readable< ::q::byte_block > >( );

		std::atomic_store( &self->writable_out_, w );
		std::atomic_store( &self->readable_out_, r );

		self->close( );
	};

	if ( detached_.exchange( true ) )
		// Already detached
		return;

	readable_out_->add_scope_until_closed(
		q::make_scoped_function( std::move( scope ) ) );
}

} } // namespace io, namespace q
