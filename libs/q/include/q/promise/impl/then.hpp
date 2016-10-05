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

#ifndef LIBQ_PROMISE_PROMISE_IMPL_THEN_HPP
#define LIBQ_PROMISE_PROMISE_IMPL_THEN_HPP

namespace q { namespace detail {

/**
 * ( ... ) -> value
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
inline typename std::enable_if<
	is_function_t< Fn >::value
	and
	!first_argument_is_tuple< Fn >::value
	and
	generic_promise<
		Shared, std::tuple< Args... >
	>::template is_valid_arguments<
		arguments_of_t< Fn >
	>::value
	and
	!is_promise< result_of_t< Fn > >::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	promise< result_of_as_tuple_t< Fn > >
>::type
generic_promise< Shared, std::tuple< Args... > >::
then( Fn&& fn, Queue&& queue )
{
	typedef result_of_as_tuple_t< Fn > return_tuple_type;
	auto deferred = detail::defer< return_tuple_type >::construct(
		is_set_default< Queue >::value
		? ensure( set_default_get( queue ) )
		: get_queue( ) );
	Q_MAKE_MOVABLE( fn );
	auto state = state_;

	auto perform = [ deferred, Q_MOVABLE_MOVE( fn ), state ]( ) mutable
	{
		auto value = state->consume( );
		if ( value.has_exception( ) )
			// Redirect exception
			deferred->set_exception( value.exception( ) );
		else
			deferred->set_by_fun(
				Q_MOVABLE_CONSUME( fn ), value.consume( ) );
	};

	state_->signal( )->push( std::move( perform ),
	                         ensure( set_default_forward( queue ) ) );

	return std::move( deferred->get_promise( ) );
}

/**
 * ( std::tuple< ... > ) -> value
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	first_argument_is_tuple< Fn >::value
	and
	::q::is_argument_same_or_convertible_incl_void<
		arguments< Args... >,
		typename tuple_arguments<
			first_argument_of_t< Fn >
		>::type
	>::value
	and
	!is_promise< result_of_t< Fn > >::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	promise< result_of_as_tuple_t< Fn > >
>::type
generic_promise< Shared, std::tuple< Args... > >::
then( Fn&& fn, Queue&& queue )
{
	typedef result_of_as_tuple_t< Fn > return_tuple_type;
	auto deferred = detail::defer< return_tuple_type >::construct(
	      is_set_default< Queue >::value
	      ? ensure( set_default_get( queue ) )
	      : get_queue( ) );
	Q_MAKE_MOVABLE( fn );
	auto state = state_;

	auto perform = [ deferred, Q_MOVABLE_MOVE( fn ), state ]( ) mutable
	{
		auto value = state->consume( );
		if ( value.has_exception( ) )
			// Redirect exception
			deferred->set_exception( value.exception( ) );
		else
			deferred->set_by_fun(
				Q_MOVABLE_CONSUME( fn ), value.consume( ) );
	};

	state_->signal( )->push( std::move( perform ),
	                         ensure( set_default_forward( queue ) ) );

	return std::move( deferred->get_promise( ) );
}

/**
 * ( ... ) -> promise< value >
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	!first_argument_is_tuple< Fn >::value
	and
	generic_promise<
		Shared, std::tuple< Args... >
	>::template is_valid_arguments<
		arguments_of_t< Fn >
	>::value
	and
	is_promise< result_of_t< Fn > >::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	typename result_of_t< Fn >::unique_this_type
>::type
generic_promise< Shared, std::tuple< Args... > >::
then( Fn&& fn, Queue&& queue )
{
	typedef typename result_of_t< Fn >::tuple_type return_tuple_type;
	auto deferred = detail::defer< return_tuple_type >::construct(
	      is_set_default< Queue >::value
	      ? ensure( set_default_get( queue ) )
	      : get_queue( ) );
	Q_MAKE_MOVABLE( fn );
	auto state = state_;

	auto perform = [ deferred, Q_MOVABLE_MOVE( fn ), state ]( ) mutable
	{
		auto value = state->consume( );
		if ( value.has_exception( ) )
			// Redirect exception
			deferred->set_exception( value.exception( ) );
		else
			deferred->set_by_fun(
				Q_MOVABLE_CONSUME( fn ), value.consume( ) );
	};

	state_->signal( )->push( std::move( perform ),
	                         ensure( set_default_forward( queue ) ) );

	return std::move( deferred->get_promise( ) );
}

/**
 * ( std::tuple< ... > ) -> promise< value >
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	first_argument_is_tuple< Fn >::value
	and
	::q::is_argument_same_or_convertible_incl_void<
		arguments< Args... >,
		typename tuple_arguments<
			first_argument_of_t< Fn >
		>::type
	>::value
	and
	is_promise< result_of_t< Fn > >::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	typename result_of_t< Fn >::unique_this_type
>::type
generic_promise< Shared, std::tuple< Args... > >::
then( Fn&& fn, Queue&& queue )
{
	typedef typename result_of_t< Fn >::tuple_type return_tuple_type;
	auto deferred = detail::defer< return_tuple_type >::construct(
	      is_set_default< Queue >::value
	      ? ensure( set_default_get( queue ) )
	      : get_queue( ) );
	Q_MAKE_MOVABLE( fn );
	auto state = state_;

	auto perform = [ deferred, Q_MOVABLE_MOVE( fn ), state ]( ) mutable
	{
		auto value = state->consume( );
		if ( value.has_exception( ) )
			// Redirect exception
			deferred->set_exception( value.exception( ) );
		else
			deferred->set_by_fun(
				Q_MOVABLE_CONSUME( fn ), value.consume( ) );
	};

	state_->signal( )->push( std::move( perform ),
				ensure( set_default_forward( queue ) ) );

	return std::move( deferred->get_promise( ) );
}

template< bool Shared, typename... Args >
template< typename AsyncTask >
inline typename std::enable_if<
	is_same_type< AsyncTask, async_task >::value,
	typename generic_promise<
		Shared, std::tuple< Args... >
	>::promise_this_type
>::type
generic_promise< Shared, std::tuple< Args... > >::
then( AsyncTask&& task )
{
	auto deferred = detail::defer< std::tuple< Args... > >::construct(
		queue_ );
	auto state = state_;
	Q_MAKE_MOVABLE( task );

	auto perform = [ Q_MOVABLE_MOVE( task ), deferred, state ]( ) mutable
	{
		auto value = state->consume( );

		if ( value.has_exception( ) )
		{
			deferred->set_exception( value.exception( ) );
		}
		else
		{
			Q_MAKE_MOVABLE( value );
			auto runner = [ Q_MOVABLE_MOVE( value ), deferred ]
				( q::expect< > expect ) mutable noexcept
			{
				if ( expect.has_exception( ) )
				{
					deferred->set_exception(
						expect.exception( ) );
				}
				else
				{
					deferred->set_expect(
						Q_MOVABLE_CONSUME( value ) );
				}
			};

			Q_MOVABLE_CONSUME( task ).run( runner );
		}
	};

	state_->signal( )->push_synchronous( std::move( perform ) );

	return deferred->template get_suitable_promise< promise_this_type >( );
}

} } // namespace detail, namespace q

#endif // LIBQ_PROMISE_PROMISE_IMPL_THEN_HPP
