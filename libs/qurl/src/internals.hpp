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

#ifndef LIBQURL_INTERNAL_INTERNALS_HPP
#define LIBQURL_INTERNAL_INTERNALS_HPP

#include <qurl/context.hpp>

#include <curl/curl.h>

#include "curl_timeout.hpp"
#include "curl_socket.hpp"

#include <unordered_map>

namespace qurl {

struct context::pimpl
{
	q::io::dispatcher_ptr dispatcher_;
	CURLM* multi_handle_;

	context_weak_ptr* weak_context_socket_;
	context_weak_ptr* weak_context_timeout_;

	curl_timeout_ptr timeout_;
	std::unordered_map< ::q::io::socket_t, curl_socket_ptr > sockets_;
};

} // namespace qurl

#endif // LIBQURL_INTERNAL_INTERNALS_HPP
