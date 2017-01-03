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

#include <q-io/udp_receiver.hpp>

#include "impl/udp_receiver.hpp"

namespace q { namespace io {

udp_receiver_ptr
udp_receiver::construct( std::shared_ptr< udp_receiver::pimpl >&& pimpl )
{
	return q::make_shared_using_constructor< udp_receiver >(
		std::move( pimpl ) );
}

udp_receiver::udp_receiver( std::shared_ptr< udp_receiver::pimpl >&& pimpl )
: pimpl_( std::move( pimpl ) )
{ }

udp_receiver::~udp_receiver( )
{
	// continue if detached, otherwise close
	if ( !pimpl_->detached_ )
		pimpl_->close( );
}

q::readable< udp_packet > udp_receiver::get_readable( )
{
	return *pimpl_->readable_in_;
}

void udp_receiver::detach( )
{
	pimpl_->detach( );
}

} } // namespace io, namespace q
