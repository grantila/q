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

#ifndef LIBQ_PROMISE_PROMISE_IMPL_FINALLY_HPP
#define LIBQ_PROMISE_PROMISE_IMPL_FINALLY_HPP

namespace q { namespace detail {

/**
 * void -> void
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	std::is_void< result_of_t< Fn > >::value
	and
	arity_of_t< Fn >::value == 0
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	typename generic_promise< Shared, Args... >::promise_this_type
>::type
generic_promise< Shared, Args... >::
finally( Fn&& fn, Queue&& queue )
{
	auto deferred = ::q::make_shared< detail::defer< Args... > >(
	     is_set_default< Queue >::value
	     ? ensure( set_default_get( queue ) )
	     : get_queue( ) );
	Q_MAKE_MOVABLE( fn );
	auto state = state_;

	auto perform = [ deferred, Q_MOVABLE_FORWARD( fn ), state ]( ) mutable
	{
		auto value = state.consume( );

		try
		{
			Q_MOVABLE_CONSUME( fn )( );

			deferred->set_expect( std::move( value ) );
		}
		catch ( ... )
		{
			// TODO: Consider using a nested_exception
			deferred->set_exception( std::current_exception( ) );
		}
	};

	state_.signal( ).push( std::move( perform ),
	                         ensure( set_default_forward( queue ) ) );

	return deferred->template get_suitable_promise< promise_this_type >( );
}

/**
 * void -> P< >
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	::q::is_promise< result_of_t< Fn > >::value
	and
	result_of_t< Fn >::argument_types::empty::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	typename generic_promise< Shared, Args... >::promise_this_type
>::type
generic_promise< Shared, Args...>::
finally( Fn&& fn, Queue&& queue )
{
	auto deferred = ::q::make_shared< detail::defer< Args... > >(
	     is_set_default< Queue >::value
	     ? ensure( set_default_get( queue ) )
	     : get_queue( ) );
	Q_MAKE_MOVABLE( fn );
	auto state = state_;

	auto perform = [ deferred, Q_MOVABLE_FORWARD( fn ), state ]( ) mutable
	{
		try
		{
			auto inner_promise = Q_MOVABLE_CONSUME( fn )( );

			inner_promise
			.then( [ deferred, state ]( ) mutable
			{
				auto value = state.consume( );

				deferred->set_expect( std::move( value ) );
			} )
			.fail( [ deferred, state ]( std::exception_ptr&& e )
			mutable
			{
				auto value = state.consume( );

				// TODO: Consider using a nested_exception for
				//       inner exceptions.
				deferred->set_exception( std::move( e ) );

				// deferred->set_expect( std::move( value ) );
			} );
		}
		catch ( ... )
		{
			// TODO: Consider using a nested_exception
			deferred->set_exception( std::current_exception( ) );
		}
	};

	state_.signal( ).push( std::move( perform ),
	                         ensure( set_default_forward( queue ) ) );

	return deferred->get_promise( );
}

} } // namespace detail, namespace q

#endif // LIBQ_PROMISE_PROMISE_IMPL_FINALLY_HPP
