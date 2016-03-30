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

#ifndef LIBQURL_TYPES_HPP
#define LIBQURL_TYPES_HPP

#include <q/exception.hpp>

#include <memory>

namespace qurl {

Q_MAKE_SIMPLE_EXCEPTION( exception );

class context;
typedef std::shared_ptr< context > context_ptr;
typedef std::weak_ptr< context > context_weak_ptr;

class handle;
typedef std::shared_ptr< handle > handle_ptr;
typedef std::weak_ptr< handle > handle_weak_ptr;

} // namespace qurl

#endif // LIBQURL_TYPES_HPP
