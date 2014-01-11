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

#ifndef LIBQ_PROMISE_REJECT_HPP
#define LIBQ_PROMISE_REJECT_HPP

#include <q/temporarily_copyable.hpp>
#include <q/type_traits.hpp>

#include <algorithm>

namespace q {

/**
 * Returns an immediately rejected promise with exception, either as a
 * std::exception_ptr or as an exception object.
 */
template< typename Arguments, typename E >
typename std::enable_if<
	is_same_type< E, std::exception_ptr >::value,
	promise< typename Arguments::tuple_type >
>::type
reject( E&& e )
{
	typedef typename Arguments::template apply< detail::defer >::type
		defer_type;

	auto deferred = ::q::make_shared< defer_type >( );

	deferred->set_exception( std::forward< E >( e ) );

	return deferred->get_promise( );
}

template< typename Arguments, typename E >
typename std::enable_if<
	!is_same_type< E, std::exception_ptr >::value,
	promise< typename Arguments::tuple_type >
>::type
reject( E&& e )
{
	typedef typename Arguments::template apply< detail::defer >::type
		defer_type;

	auto deferred = ::q::make_shared< defer_type >( );

	deferred->set_exception(
		std::make_exception_ptr( std::forward< E >( e ) ) );

	return deferred->get_promise( );
}

} // namespace q

#endif // LIBQ_PROMISE_REJECT_HPP
