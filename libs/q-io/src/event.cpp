/*
 * Copyright 2014 Gustaf Räntilä
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

#include <q-io/event.hpp>

#include "internals.hpp"

#include <event2/event.h>

namespace q { namespace io {

event::event( )
: pimpl_( new pimpl{ nullptr } )
{
	;
}

event::~event( )
{
	if ( pimpl_->ev )
		event_free( pimpl_->ev );
}

dispatcher::pimpl& event::get_dispatcher_pimpl( )
{
	return *pimpl_->dispatcher.lock( )->pimpl_;
}

dispatcher_ptr event::get_dispatcher( )
{
	return pimpl_->dispatcher.lock( );
}

void event::attach( const dispatcher_ptr& dispatcher )
{
	pimpl_->dispatcher = dispatcher;

	sub_attach( dispatcher );
}

} } // namespace io, namespace q
