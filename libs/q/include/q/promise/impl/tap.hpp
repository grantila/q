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

#ifndef LIBQ_PROMISE_PROMISE_IMPL_TAP_HPP
#define LIBQ_PROMISE_PROMISE_IMPL_TAP_HPP

namespace q { namespace detail {

/**
 * ( T... ) -> void
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	generic_promise< Shared, std::tuple< Args... > >::
		template is_valid_arguments<
			arguments_of_t< Fn >
		>::value
	and
	std::is_void< result_of_t< Fn > >::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	promise< std::tuple< Args... > >
>::type
generic_promise< Shared, std::tuple< Args... > >::
tap( Fn&& fn, Queue&& queue )
{
	auto deferred = ::q::make_shared< detail::defer< Args... > >(
	     is_set_default< Queue >::value
	     ? ensure( set_default_get( queue ) )
	     : get_queue( ) );
	Q_MAKE_MOVABLE( fn );
	auto state = state_;

	auto perform = [ deferred, Q_MOVABLE_MOVE( fn ), state ]( ) mutable
	{
		auto value = state->consume( );

		if ( value.has_exception( ) )
		{
			// Redirect exception
			deferred->set_exception( value.exception( ) );
			return;
		}

		try
		{
			call_with_args_by_tuple(
				Q_MOVABLE_CONSUME( fn ),
				value.get( )
			);

			deferred->set_value( value.consume( ) );
		}
		catch ( ... )
		{
			deferred->set_exception( std::current_exception( ) );
		}
	};

	state_->signal( )->push( std::move( perform ),
	                         ensure( set_default_forward( queue ) ) );

	return std::move( deferred->get_promise( ) );
}

/**
 * ( T... ) -> P< >
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	generic_promise< Shared, std::tuple< Args... > >::
		template is_valid_arguments<
			arguments_of_t< Fn >
		>::value
	and
	is_promise< result_of_t< Fn > >::value
	and
	result_of_t< Fn >::argument_types::empty_or_voidish::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	promise< std::tuple< Args... > >
>::type
generic_promise< Shared, std::tuple< Args... > >::
tap( Fn&& fn, Queue&& queue )
{
	auto deferred = ::q::make_shared< detail::defer< Args... > >(
	      is_set_default< Queue >::value
	      ? ensure( set_default_get( queue ) )
	      : get_queue( ) );
	Q_MAKE_MOVABLE( fn );
	auto state = state_;

	auto perform = [ deferred, Q_MOVABLE_MOVE( fn ), state ]( ) mutable
	{
		auto value = state->consume( );

		if ( value.has_exception( ) )
		{
			// Redirect exception
			deferred->set_exception( value.exception( ) );
			return;
		}

		try
		{
			auto shared_state = std::make_shared<
				decltype( value )
			>( std::move( value ) );

			q::call_with_args_by_tuple(
				Q_MOVABLE_CONSUME( fn ), shared_state->get( )
			)
			.then( [ deferred, shared_state ]( )
			{
				deferred->set_value( shared_state->consume( ) );
			} )
			.fail( [ deferred ]( std::exception_ptr e )
			{
				deferred->set_exception( e );
			} );
		}
		catch ( ... )
		{
			deferred->set_exception( std::current_exception( ) );
		}
	};

	state_->signal( )->push( std::move( perform ),
	                         ensure( set_default_forward( queue ) ) );

	return std::move( deferred->get_promise( ) );
}

/**
 * ( std::tuple< ... > ) -> void
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	first_argument_is_tuple< Fn >::value
	and
	::q::is_argument_same_or_convertible_incl_void_t<
		arguments< Args... >,
		tuple_arguments_t< first_argument_of_t< Fn > >
	>::value
	and
	std::is_void< result_of_t< Fn > >::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	promise< std::tuple< Args... > >
>::type
generic_promise< Shared, std::tuple< Args... > >::
tap( Fn&& fn, Queue&& queue )
{
	auto deferred = ::q::make_shared< detail::defer< Args... > >(
	     is_set_default< Queue >::value
	     ? ensure( set_default_get( queue ) )
	     : get_queue( ) );
	Q_MAKE_MOVABLE( fn );
	auto state = state_;

	auto perform = [ deferred, Q_MOVABLE_MOVE( fn ), state ]( ) mutable
	{
		auto value = state->consume( );

		if ( value.has_exception( ) )
		{
			// Redirect exception
			deferred->set_exception( value.exception( ) );
			return;
		}

		try
		{
			call_with_args(
				Q_MOVABLE_CONSUME( fn ),
				value.get( )
			);

			deferred->set_value( value.consume( ) );
		}
		catch ( ... )
		{
			deferred->set_exception( std::current_exception( ) );
		}
	};

	state_->signal( )->push( std::move( perform ),
	                         ensure( set_default_forward( queue ) ) );

	return std::move( deferred->get_promise( ) );
}

/**
 * ( std::tuple< ... > ) -> P< >
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	first_argument_is_tuple< Fn >::value
	and
	::q::is_argument_same_or_convertible_incl_void_t<
		arguments< Args... >,
		tuple_arguments_t< first_argument_of_t< Fn > >
	>::value
	and
	is_promise< result_of_t< Fn > >::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	promise< std::tuple< Args... > >
>::type
generic_promise< Shared, std::tuple< Args... > >::
tap( Fn&& fn, Queue&& queue )
{
	auto deferred = ::q::make_shared< detail::defer< Args... > >(
	      is_set_default< Queue >::value
	      ? ensure( set_default_get( queue ) )
	      : get_queue( ) );
	Q_MAKE_MOVABLE( fn );
	auto state = state_;

	auto perform = [ deferred, Q_MOVABLE_MOVE( fn ), state ]( ) mutable
	{
		auto value = state->consume( );

		if ( value.has_exception( ) )
		{
			// Redirect exception
			deferred->set_exception( value.exception( ) );
			return;
		}

		try
		{
			auto shared_state = std::make_shared<
				decltype( value )
			>( std::move( value ) );

			q::call_with_args(
				Q_MOVABLE_CONSUME( fn ), shared_state->get( )
			)
			.then( [ deferred, shared_state ]( )
			{
				deferred->set_value( shared_state->consume( ) );
			} )
			.fail( [ deferred ]( std::exception_ptr e )
			{
				deferred->set_exception( e );
			} );
		}
		catch ( ... )
		{
			deferred->set_exception( std::current_exception( ) );
		}
	};

	state_->signal( )->push( std::move( perform ),
	                         ensure( set_default_forward( queue ) ) );

	return std::move( deferred->get_promise( ) );
}

} } // namespace detail, namespace q

#endif // LIBQ_PROMISE_PROMISE_IMPL_TAP_HPP
