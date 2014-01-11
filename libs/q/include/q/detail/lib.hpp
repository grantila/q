/*
 * Copyright 2013 Gustaf Räntilä
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

#ifndef LIBQ_DETAIL_LIB_HPP
#define LIBQ_DETAIL_LIB_HPP

namespace q { namespace detail {

typedef void( *uncaught_exception_handler_fn )( const std::exception_ptr& );

void register_uncaught_exception_handler( uncaught_exception_handler_fn e );

void handle_uncaught_exception( const std::exception_ptr& e );

#define LIBQ_UNCAUGHT_EXCEPTION( e ) ::q::detail::handle_uncaught_exception( e )

/**
 * @returns the filename of the shared object (executable, .so, .exe or .dll)
 * of Q. This name is different depending on platform and shared vs dynamic
 * linking of Q itself.
 */
const char* shared_object( );

} } // namespace detail, namespace q

#endif // LIBQ_DETAIL_LIB_HPP
