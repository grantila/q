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

#include <q-io/tcp_socket.hpp>

#include <q/scope.hpp>

#include "impl/tcp_socket.hpp"

namespace q { namespace io {

tcp_socket::tcp_socket( std::shared_ptr< tcp_socket::pimpl >&& _pimpl )
: pimpl_( std::move( _pimpl ) )
{
	pimpl_->self_ = pimpl_;
}

tcp_socket::~tcp_socket( )
{
	if ( !!pimpl_->readable_in_ )
		// Not detached - force close the socket right now.
		pimpl_->close( );
}

tcp_socket_ptr
tcp_socket::construct( std::shared_ptr< tcp_socket::pimpl >&& pimpl )
{
	return q::make_shared_using_constructor< tcp_socket >(
		std::move( pimpl ) );
}


q::readable< q::byte_block > tcp_socket::in( )
{
	return *pimpl_->readable_in_;
}

q::writable< q::byte_block > tcp_socket::out( )
{
	return *pimpl_->writable_out_;
}

void tcp_socket::detach( )
{
	// Remove references to the user-ends of the channels, and let them own
	// these sides of the socket. When they are destructed, they'll close
	// the channels, and we can adapt to this, and eventually destruct when
	// all data is sent.

	auto in = std::atomic_load( &pimpl_->readable_in_ );
	auto out = std::atomic_load( &pimpl_->writable_out_ );

	if ( !in )
		// Already detached (probably)
		return;

	in->add_scope_until_closed(
		::q::make_scope( shared_from_this( ) ) );
	out->add_scope_until_closed(
		::q::make_scope( shared_from_this( ) ) );

	in.reset( );
	out.reset( );

	std::atomic_store( &pimpl_->readable_in_, in );
	std::atomic_store( &pimpl_->writable_out_, out );
}

} } // namespace io, namespace q
