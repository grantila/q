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

#ifndef LIBQ_PROMISE_PROMISE_IMPL_FAIL_HPP
#define LIBQ_PROMISE_PROMISE_IMPL_FAIL_HPP

namespace q { namespace detail {

/**
 * std::exception_ptr -> tuple< T... >
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	arity_of_t< Fn >::value == 1
	and
	is_same_type<
		first_argument_of_t< Fn >,
		std::exception_ptr
	>::value
	and
	tuple_arguments_t< result_of_t< Fn > >
		::template is_convertible_to< arguments< Args... > >::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	typename generic_promise< Shared, Args... >::promise_this_type
>::type
generic_promise< Shared, Args... >::
fail( Fn&& fn, Queue&& queue )
{
	auto deferred = detail::defer< Args... >::construct(
	    is_set_default< Queue >::value
	    ? ensure( set_default_get( queue ) )
	    : get_queue( ) );
	Q_MAKE_MOVABLE( fn );
	auto state = state_;

	auto perform = [ deferred, Q_MOVABLE_FORWARD( fn ), state ]( ) mutable
	{
		auto value = state.consume( );
		if ( value.has_exception( ) )
		{
			// Redirect exception
			try
			{
				deferred->set_by_fun(
					Q_MOVABLE_CONSUME( fn ),
					value.exception( ) );
			}
			catch ( ... )
			{
				deferred->set_exception(
					std::current_exception( ) );
				return;
			}
		}
		else
		{
			// Forward data
			deferred->set_value( value.consume( ) );
		}
	};

	state_.signal( ).push( std::move( perform ),
	                         ensure( set_default_forward( queue ) ) );

	return deferred->template get_suitable_promise< promise_this_type >( );
}

/**
 * std::exception_ptr -> P< tuple< T... > >
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	is_same_type<
		first_argument_of_t< Fn >,
		std::exception_ptr
	>::value
	and
	is_promise< result_of_t< Fn > >::value
	and
	result_of_t< Fn >::argument_types
		::template is_convertible_to< arguments< Args... > >::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	typename generic_promise< Shared, Args... >::promise_this_type
>::type
generic_promise< Shared, Args... >::
fail( Fn&& fn, Queue&& queue )
{
	auto deferred = detail::defer< tuple_type >::construct(
	       is_set_default< Queue >::value
	       ? ensure( set_default_get( queue ) )
	       : get_queue( ) );
	Q_MAKE_MOVABLE( fn );
	auto state = state_;

	auto perform = [ deferred, Q_MOVABLE_FORWARD( fn ), state ]( ) mutable
	{
		auto value = state.consume( );
		if ( value.has_exception( ) )
		{
			// Redirect exception
			try
			{
				deferred->set_by_fun(
					Q_MOVABLE_CONSUME( fn ),
					value.exception( ) );
			}
			catch ( ... )
			{
				deferred->set_exception(
					std::current_exception( ) );
			}
		}
		else
		{
			// Forward data
			deferred->set_value( value.consume( ) );
		}
	};

	state_.signal( ).push( std::move( perform ),
	                         ensure( set_default_forward( queue ) ) );

	return deferred->template get_suitable_promise< promise_this_type >( );
}

/**
 * E -> tuple< T... >
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	arity_of_t< Fn >::value == 1
	and
	!arguments_of_are_t< Fn, std::exception_ptr >::value
	and
	tuple_arguments_t< result_of_t< Fn > >
		::template is_convertible_to< arguments< Args... > >::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	typename generic_promise< Shared, Args... >::promise_this_type
>::type
generic_promise< Shared, Args... >::
fail( Fn&& fn, Queue&& queue )
{
	auto deferred = detail::defer< tuple_type >::construct(
	       is_set_default< Queue >::value
	       ? ensure( set_default_get( queue ) )
	       : get_queue( ) );
	Q_MAKE_MOVABLE( fn );
	auto state = state_;

	typedef typename std::decay< Q_FIRST_ARGUMENT_OF( Fn ) >::type
		exception_type;

	auto perform = [ deferred, Q_MOVABLE_FORWARD( fn ), state ]( ) mutable
	{
		auto value = state.consume( );
		if ( value.has_exception( ) )
		{
			// Handle exception, if it's our type
			try
			{
				std::rethrow_exception( value.exception( ) );
			}
			catch ( exception_type& e )
			{
				try
				{
					deferred->set_by_fun(
						Q_MOVABLE_CONSUME( fn ), e );
				}
				catch ( ... )
				{
					deferred->set_exception(
						std::current_exception( ) );
				}
			}
			catch ( ... )
			{
				deferred->set_exception(
					std::current_exception( ) );
			}
		}
		else
		{
			// Forward data
			deferred->set_value( value.consume( ) );
		}
	};

	state_.signal( ).push( std::move( perform ),
	                         ensure( set_default_forward( queue ) ) );

	return deferred->template get_suitable_promise< promise_this_type >( );
}

/**
 * E -> P< tuple< T... > >
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	arity_of_t< Fn >::value == 1
	and
	!arguments_of_are_t< Fn, std::exception_ptr >::value
	and
	is_promise< result_of_t< Fn > >::value
	and
	result_of_t< Fn >::argument_types
		::template is_convertible_to< arguments< Args... > >::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	typename generic_promise< Shared, Args... >::promise_this_type
>::type
generic_promise< Shared, Args... >::
fail( Fn&& fn, Queue&& queue )
{
	auto deferred = detail::defer< tuple_type >::construct(
	       is_set_default< Queue >::value
	       ? ensure( set_default_get( queue ) )
	       : get_queue( ) );
	Q_MAKE_MOVABLE( fn );
	auto state = state_;

	typedef typename std::decay< Q_FIRST_ARGUMENT_OF( Fn ) >::type
		exception_type;

	auto perform = [ deferred, Q_MOVABLE_FORWARD( fn ), state ]( ) mutable
	{
		auto value = state.consume( );
		if ( value.has_exception( ) )
		{
			// Handle exception, if it's our type
			try
			{
				std::rethrow_exception( value.exception( ) );
			}
			catch ( exception_type& e )
			{
				try
				{
					deferred->set_by_fun(
						Q_MOVABLE_CONSUME( fn ), e );
				}
				catch ( ... )
				{
					deferred->set_exception(
						std::current_exception( ) );
				}
			}
			catch ( ... )
			{
				deferred->set_exception(
					std::current_exception( ) );
			}
		}
		else
		{
			// Forward data
			deferred->set_value( value.consume( ) );
		}
	};

	state_.signal( ).push( std::move( perform ),
	                         ensure( set_default_forward( queue ) ) );

	return deferred->template get_suitable_promise< promise_this_type >( );
}

} } // namespace detail, namespace q

#endif // LIBQ_PROMISE_PROMISE_IMPL_FAIL_HPP
