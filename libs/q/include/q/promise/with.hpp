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

#ifndef LIBQ_PROMISE_WITH_HPP
#define LIBQ_PROMISE_WITH_HPP

#include <q/memory.hpp>

namespace q {

/**
 * Main entry point for starting an asynchronous chain of events based on a
 * set of values (which will be injected as function arguments to the
 * subsequent then() calls.)
 */
template< typename... T >
promise< std::tuple< T... > >
with( T&&... t )
{
	auto deferred = ::q::make_shared< detail::defer< T... > >( );

	deferred->set_value( std::forward_as_tuple( t... ) );

	return deferred->get_promise( );
}

} // namespace q

#endif // LIBQ_PROMISE_WITH_HPP
