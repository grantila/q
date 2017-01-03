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

#include <q-io/udp_sender.hpp>

#include "impl/udp_sender.hpp"

namespace q { namespace io {

udp_sender_ptr
udp_sender::construct( std::shared_ptr< udp_sender::pimpl >&& pimpl )
{
	return q::make_shared_using_constructor< udp_sender >(
		std::move( pimpl ) );
}

udp_sender::udp_sender( std::shared_ptr< udp_sender::pimpl >&& pimpl )
: pimpl_( std::move( pimpl ) )
{
}

udp_sender::~udp_sender( )
{
	// continue if detached, otherwise close
	if ( !pimpl_->detached_ )
		pimpl_->close( );
}

q::writable< q::byte_block > udp_sender::get_writable( )
{
	return *pimpl_->writable_out_;
}

void udp_sender::detach( )
{
	return pimpl_->detach( );
}

} } // namespace io, namespace q
