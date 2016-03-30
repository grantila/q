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

#ifndef LIBQURL_INTERNAL_CURL_SOCKET_HPP
#define LIBQURL_INTERNAL_CURL_SOCKET_HPP

#include <qurl/types.hpp>

#include <q-io/socket_event.hpp>

namespace qurl {

class curl_socket;
typedef std::shared_ptr< curl_socket > curl_socket_ptr;

class curl_socket
: public std::enable_shared_from_this< curl_socket >
, public ::q::io::socket_event
{
public:
	curl_socket( context_ptr context, ::q::io::socket_t sock );
	~curl_socket( );

	void detect_readability( )
	{
		::q::io::socket_event::detect_readability( );
	}

	void detect_writability( )
	{
		::q::io::socket_event::detect_writability( );
	}

protected:
	void on_event_read( ) noexcept override;
	void on_event_write( ) noexcept override;

private:
	::q::io::socket_event_ptr socket_event_shared_from_this( ) override
	{
		return shared_from_this( );
	}

	context_weak_ptr weak_context_;
};

} // namespace qurl

#endif // LIBQURL_INTERNAL_CURL_SOCKET_HPP
