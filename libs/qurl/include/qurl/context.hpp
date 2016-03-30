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

#ifndef LIBQURL_CONTEXT_HPP
#define LIBQURL_CONTEXT_HPP

#include <qurl/types.hpp>

#include <q-io/dispatcher.hpp>

namespace qurl {

class context
: public std::enable_shared_from_this< context >
{
public:
	~context( );

	static context_ptr construct( ::q::io::dispatcher_ptr dispatcher );

	handle_ptr create_handle( );

protected:
	context( ::q::io::dispatcher_ptr dispatcher );

private:
	friend class curl_timeout;
	friend class curl_socket;
	friend class handle;

	void _init( );

	void on_readable( ::q::io::socket_t sock ) noexcept;
	void on_writable( ::q::io::socket_t sock ) noexcept;

	void on_timeout( ) noexcept;

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

} // namespace qurl

#endif // LIBQURL_CONTEXT_HPP
