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

#include "curl_timeout.hpp"
#include "internals.hpp"

namespace qurl {

std::shared_ptr< curl_timeout > curl_timeout::construct( context_ptr ctx )
{
	auto self = q::make_shared_using_constructor< curl_timeout>( ctx );

	ctx->pimpl_->dispatcher_->attach_event( self );

	return self;
}

curl_timeout::curl_timeout( context_ptr ctx )
: context_( std::move( ctx ) )
{ }

void curl_timeout::on_event_timeout( ) noexcept
{
	auto ctx = context_.lock( );

	if ( !!ctx )
		ctx->on_timeout( );
}

} // namespace qurl
