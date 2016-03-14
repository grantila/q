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

#ifndef LIBQIO_TYPES_HPP
#define LIBQIO_TYPES_HPP

#include <memory>

namespace q { namespace io {

class dispatcher;
typedef std::shared_ptr< dispatcher > dispatcher_ptr;

class event;
typedef std::shared_ptr< event > event_ptr;

class socket;
typedef std::shared_ptr< socket > socket_ptr;
typedef std::weak_ptr< socket > weak_socket_ptr;

class server_socket;
typedef std::shared_ptr< server_socket > server_socket_ptr;

struct native_socket;

class resolver;

} } // namespace io, namespace q

#endif // LIBQIO_TYPES_HPP
