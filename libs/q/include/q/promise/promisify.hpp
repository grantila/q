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

#ifndef LIBQ_PROMISE_PROMISIFY_HPP
#define LIBQ_PROMISE_PROMISIFY_HPP

namespace q {

namespace detail {

template< typename Ret, typename Args >
struct promisifier;

template< typename Ret, typename... Args >
struct promisifier< Ret, ::q::arguments< Args... > >
{
	typedef typename std::conditional<
		std::is_same< Ret, void >::value,
		::q::arguments< >,
		::q::arguments< Ret >
	>::type arguments_type;
	typedef typename arguments_type::tuple_type tuple_type;
	typedef typename arguments_type::template apply< defer > defer_type;

	template< typename Fn >
	static q::function< promise< tuple_type >( Args&&... ) >
	promisify( queue_ptr&& queue, Fn fn )
	{
		Q_MAKE_MOVABLE( fn );

		return q::unique_function< promise< tuple_type >( Args&&... ) >(
			[ queue, Q_MOVABLE_FORWARD( fn ) ]( Args&&... args )
			mutable
			-> promise< tuple_type >
			{
				auto deferred = defer_type::construct( queue );

				deferred->set_by_fun(
					Q_MOVABLE_GET( fn ), args... );

				return deferred->get_promise( );
			}
		)
		.share( );
	}
};

} // namespace detail

/**
 * Converts a function returning a value, into a function returning a promise
 */
template< typename Fn >
typename std::enable_if<
	!is_promise< Q_RESULT_OF( Fn ) >::value,
	decltype(
		detail::promisifier< result_of_t< Fn >, arguments_of_t< Fn > >
		::promisify( nullptr, std::declval< Fn >( ) )
	)
>::type
promisify( queue_ptr queue, Fn&& fn )
{
	return detail::promisifier< result_of_t< Fn >, arguments_of_t< Fn > >
		::promisify( std::move( queue ), std::forward< Fn >( fn ) );
}

// No-op fallback for already promise-returning functions
template< typename Fn >
typename std::enable_if< is_promise< Q_RESULT_OF( Fn ) >::value, Fn >::type
promisify( queue_ptr, Fn&& fn )
{
	return fn;
}

} // namespace q

#endif // LIBQ_PROMISE_PROMISIFY_HPP
