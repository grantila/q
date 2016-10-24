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
promise<
	typename decayed_tuple<
		typename std::decay< T >::type...
	>::type
>
with( const queue_ptr& queue, T&&... t )
{
	typedef detail::defer<
		typename std::decay< T >::type...
	> defer_type;
	auto deferred = ::q::make_shared< defer_type >( queue );

	deferred->set_value( std::forward_as_tuple( std::forward< T >( t )... ) );

	return deferred->get_promise( );
}

/**
 * Specialization for tuples
 */
template< typename Tuple >
typename std::enable_if<
	is_tuple< typename std::decay< Tuple >::type >::value,
	promise< typename std::decay< Tuple >::type >
>::type
with( const queue_ptr& queue, Tuple&& t )
{
	typedef typename tuple_arguments_t< typename std::decay< Tuple >::type >
		::template apply< detail::defer > defer_type;
	auto deferred = ::q::make_shared< defer_type >( queue );

	deferred->set_value( std::forward< Tuple >( t ) );

	return deferred->get_promise( );
}

/**
 * Specialization for promises
 */
template< typename Promise >
typename std::enable_if<
	is_promise< typename std::decay< Promise >::type >::value,
	typename std::decay< Promise >::type
>::type
with( const queue_ptr& queue, Promise&& p )
{
	return p.use_queue( queue );
}

} // namespace q

#endif // LIBQ_PROMISE_WITH_HPP
