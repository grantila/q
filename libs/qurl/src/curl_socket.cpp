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

#include "curl_socket.hpp"

#include <qurl/context.hpp>

namespace qurl {

curl_socket::curl_socket( context_ptr context, ::q::io::socket_t sock )
: ::q::io::socket_event( sock )
, weak_context_( std::move( context ) )
{ }

curl_socket::~curl_socket( )
{ }

void curl_socket::on_event_read( ) noexcept
{
	auto ctx = weak_context_.lock( );
	if ( ctx )
		ctx->on_readable( get_socket( ) );
}

void curl_socket::on_event_write( ) noexcept
{
	auto ctx = weak_context_.lock( );
	if ( ctx )
		ctx->on_writable( get_socket( ) );
}

} // namespace qurl
