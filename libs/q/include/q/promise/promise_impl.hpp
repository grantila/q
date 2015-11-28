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

#ifndef LIBQ_PROMISE_PROMISE_IMPL_HPP
#define LIBQ_PROMISE_PROMISE_IMPL_HPP

namespace q { namespace detail {

/**
 * ( ... ) -> value
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
inline typename std::enable_if<
	generic_promise<
		Shared, std::tuple< Args... >
	>::this_type::template is_valid_arguments<
		Q_ARGUMENTS_OF( Fn )
	>::value
	&&
	!is_promise< Q_RESULT_OF( Fn ) >::value
	&&
	detail::temporary< queue_ptr >::template is_decayed< Queue >::value,
	promise< Q_RESULT_OF_AS_TUPLE_TYPE( Fn ) >
>::type
generic_promise< Shared, std::tuple< Args... > >::
then( Fn&& fn, Queue&& queue )
{
	typedef Q_RESULT_OF_AS_TUPLE_TYPE( Fn ) return_tuple_type;
	auto deferred = detail::defer< return_tuple_type >::construct(
		is_temporary< Queue >::value
		? get_queue( )
		: ensure( temporary_get( queue ) ) );
	auto tmp_fn = Q_TEMPORARILY_COPYABLE( fn );
	auto state = state_;

	auto perform = [ deferred, tmp_fn, state ]( ) mutable
	{
		auto value = state->consume( );
		if ( value.has_exception( ) )
			// Redirect exception
	 		deferred->set_exception( value.exception( ) );
	 	else
			deferred->set_by_fun( tmp_fn.consume( ), value.consume( ) );
	};

	state_->signal( )->push( std::move( perform ),
	                         ensure( temporary_forward( queue ) ) );

	return std::move( deferred->get_promise( ) );
}

/**
 * TODO: Possible merge:
 * ( ... ) -> value
 * with
 * ( std::tuple< ... > ) -> value
 * due to q::defer handling it.
 */


/**
 * ( std::tuple< ... > ) -> value
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	::q::is_argument_same_or_convertible<
		arguments< typename generic_promise<
			Shared, std::tuple< Args... >
		>::tuple_type >,
		Q_ARGUMENTS_OF( Fn )
	>::value
	&&
	!is_promise< Q_RESULT_OF( Fn ) >::value
	&&
	detail::temporary< queue_ptr >::template is_decayed< Queue >::value,
	promise< Q_RESULT_OF_AS_ARGUMENT_TYPE( Fn )::tuple_type >
>::type
generic_promise< Shared, std::tuple< Args... > >::
then( Fn&& fn, Queue&& queue )
{
	typedef Q_RESULT_OF_AS_TUPLE_TYPE( Fn ) return_tuple_type;
	auto deferred = detail::defer< return_tuple_type >::construct(
	      is_temporary< Queue >::value
	      ? get_queue( )
	      : ensure( temporary_get( queue ) ) );
	auto tmp_fn = Q_TEMPORARILY_COPYABLE( fn );
	auto state = state_;

	auto perform = [ deferred, tmp_fn, state ]( ) mutable
	{
		auto value = state->consume( );
		if ( value.has_exception( ) )
			// Redirect exception
	 		deferred->set_exception( value.exception( ) );
	 	else
			deferred->set_by_fun( tmp_fn.consume( ), value.consume( ) );
	};

	state_->signal( )->push( std::move( perform ),
	                         ensure( temporary_forward( queue ) ) );

	return std::move( deferred->get_promise( ) );
}

/**
 * ( ... ) -> promise< value >
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	generic_promise<
		Shared, std::tuple< Args... >
	>::this_type::template is_valid_arguments<
		Q_ARGUMENTS_OF( Fn )
	>::value
	&&
	is_promise< Q_RESULT_OF( Fn ) >::value
	&&
	detail::temporary< queue_ptr >::template is_decayed< Queue >::value,
	Q_RESULT_OF( Fn )
>::type
generic_promise< Shared, std::tuple< Args... > >::
then( Fn&& fn, Queue&& queue )
{
	typedef Q_RESULT_OF( Fn )::tuple_type return_tuple_type;
	auto deferred = detail::defer< return_tuple_type >::construct(
	      is_temporary< Queue >::value
	      ? get_queue( )
	      : ensure( temporary_get( queue ) ) );
	auto tmp_fn = Q_TEMPORARILY_COPYABLE( fn );
	auto state = state_;

	auto perform = [ deferred, tmp_fn, state ]( ) mutable
	{
		auto value = state->consume( );
		if ( value.has_exception( ) )
			// Redirect exception
	 		deferred->set_exception( value.exception( ) );
	 	else
			deferred->satisfy_by_fun( tmp_fn.consume( ), value.consume( ) );
	};

	state_->signal( )->push( std::move( perform ),
	                         ensure( temporary_forward( queue ) ) );

	return std::move( deferred->get_promise( ) );
}

/**
 * ( std::tuple< ... > ) -> promise< value >
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	::q::is_argument_same_or_convertible<
		arguments< typename generic_promise<
			Shared, std::tuple< Args... >
		>::tuple_type >,
		Q_ARGUMENTS_OF( Fn )
	>::value
	&&
	is_promise< Q_RESULT_OF( Fn ) >::value
	&&
	detail::temporary< queue_ptr >::template is_decayed< Queue >::value,
	Q_RESULT_OF( Fn )
>::type
generic_promise< Shared, std::tuple< Args... > >::
then( Fn&& fn, Queue&& queue )
{
	typedef Q_RESULT_OF( Fn )::tuple_type return_tuple_type;
	auto deferred = detail::defer< return_tuple_type >::construct(
	      is_temporary< Queue >::value
	      ? get_queue( )
	      : ensure( temporary_get( queue ) ) );
	auto tmp_fn = Q_TEMPORARILY_COPYABLE( fn );
	auto state = state_;

	auto perform = [ deferred, tmp_fn, state ]( ) mutable
	{
		auto value = state->consume( );
		if ( value.has_exception( ) )
			// Redirect exception
	 		deferred->set_exception( value.exception( ) );
	 	else
			deferred->satisfy_by_fun( tmp_fn.consume( ), value.consume( ) );
	};

	state_->signal( )->push( std::move( perform ),
				ensure( temporary_forward( queue ) ) );

	return std::move( deferred->get_promise( ) );
}

template< bool Shared, typename... Args >
template< typename Logger, typename Queue >
inline typename std::enable_if<
	is_same_type< Logger, log_chain_generator >::value
	&&
	detail::temporary< queue_ptr >::template is_decayed< Queue >::value,
	typename generic_promise< Shared, std::tuple< Args... > >::this_type
>::type
generic_promise< Shared, std::tuple< Args... > >::
then( Logger&& logger, Queue&& queue )
{
	auto tmp_logger = Q_TEMPORARILY_COPYABLE( logger );

	auto fn = [ tmp_logger ]( tuple_type args ) -> void
	{
		call_with_args_by_tuple( tmp_logger.consume( ), args );
	};
	std::packaged_task< void( void ) > task; // REDO

	auto future = task.get_future( );

	// queue->push( fn );

	return this_type( std::move( future ) );
}


template< bool Shared, typename... Args >
template< typename AsyncTask >
inline typename std::enable_if<
	is_same_type< AsyncTask, async_task >::value,
	typename generic_promise< Shared, std::tuple< Args... > >::this_type
>::type
generic_promise< Shared, std::tuple< Args... > >::
then( AsyncTask&& task )
{
	auto deferred = detail::defer< std::tuple< Args... > >::construct(
		queue_ );
	auto state = state_;
	auto tmp_task = Q_TEMPORARILY_COPYABLE( task );

	std::cout << "Scheduling async_task::run" << std::endl;

	auto perform = [ tmp_task, deferred, state ]( ) mutable
	{
		std::cout << "Running async_task::run" << std::endl;

		auto value = state->consume( );

		if ( value.has_exception( ) )
		{
			deferred->set_exception( value.exception( ) );
		}
		else
		{
			auto tmp_value = Q_TEMPORARILY_COPYABLE( value );
			auto task( std::move( tmp_task.consume( ) ) );
			auto runner = [ tmp_value, deferred ]
				( q::expect< > expect ) mutable noexcept
			{
				if ( expect.has_exception( ) )
				{
					deferred->set_exception(
						expect.exception( ) );
				}
				else
				{
					auto value = tmp_value.consume( );
					deferred->set_value( value.consume( ) );
				}
			};

			task.run( runner );
		}
	};

	state_->signal( )->push_synchronous( std::move( perform ) );

	return std::move( deferred->get_promise( ) );
}


/**
 * Matches an exception as a raw std::exception_ptr
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_same_type<
		Q_FIRST_ARGUMENT_OF( Fn ),
		std::exception_ptr
	>::value &&
	std::is_void< Q_RESULT_OF( Fn ) >::value
	&&
	detail::temporary< queue_ptr >::template is_decayed< Queue >::value,
	typename generic_promise<
		Shared, std::tuple< Args... >
	>::unique_this_type
>::type
generic_promise< Shared, std::tuple< Args... > >::
fail( Fn&& fn, Queue&& queue )
{
	auto deferred = detail::defer< Args... >::construct(
	    is_temporary< Queue >::value
	    ? get_queue( )
	    : ensure( temporary_get( queue ) ) );
	auto tmp_fn = Q_TEMPORARILY_COPYABLE( fn );
	auto state = state_;

	auto perform = [ deferred, tmp_fn, state ]( ) mutable
	{
		auto value = state->consume( );
		if ( value.has_exception( ) )
		{
			// Redirect exception
			try
			{
				tmp_fn.consume( )( value.exception( ) );
			}
			catch ( ... )
			{
				deferred->set_exception( std::current_exception( ) );
				return;
			}
			// TODO: Set special value to the state, to let the chain continue
			// without running any handlers.
		}
		else
		{
			// Forward data
			deferred->set_value( value.consume( ) );
		}
	};

	state_->signal( )->push( std::move( perform ),
	                         ensure( temporary_forward( queue ) ) );

	return deferred->get_promise( );
}

/**
 * Matches an exception as a raw std::exception_ptr
 */
template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	is_same_type<
		Q_FIRST_ARGUMENT_OF( Fn ),
		std::exception_ptr
	>::value &&
	is_promise< Q_RESULT_OF( Fn ) >::value
	&&
	detail::temporary< queue_ptr >::template is_decayed< Queue >::value,
	Q_RESULT_OF( Fn )
>::type
generic_promise< Shared, std::tuple< Args... > >::
fail( Fn&& fn, Queue&& queue )
{
//	typedef Q_RESULT_OF( Fn )::tuple_type tuple_type;
	auto deferred = detail::defer< tuple_type >::construct(
	       is_temporary< Queue >::value
	       ? get_queue( )
	       : ensure( temporary_get( queue ) ) );
	auto tmp_fn = Q_TEMPORARILY_COPYABLE( fn );
	auto state = state_;

	auto perform = [ deferred, tmp_fn, state ]( ) mutable
	{
		auto value = state->consume( );
		if ( value.has_exception( ) )
		{
			// Redirect exception
			deferred->satisfy_by_fun(
				tmp_fn.consume( ),
				value.exception( )
			);
		}
		else
		{
			// Forward data
			deferred->set_value( value.consume( ) );
		}
	};

	state_->signal( )->push( std::move( perform ),
	                         ensure( temporary_forward( queue ) ) );

	return deferred->template get_suitable_promise< Q_RESULT_OF( Fn ) >( );
}

template< bool Shared, typename... Args >
template< typename Fn, typename Queue >
typename std::enable_if<
	std::is_void< Q_RESULT_OF( Fn ) >::value &&
	Q_ARITY_OF( Fn ) == 0
	&&
	detail::temporary< queue_ptr >::template is_decayed< Queue >::value,
	typename generic_promise<
		Shared, std::tuple< Args... >
	>::unique_this_type
>::type
generic_promise< Shared, std::tuple< Args... > >::
finally( Fn&& fn, Queue&& queue )
{
	auto deferred = ::q::make_shared< detail::defer< Args... > >(
	     is_temporary< Queue >::value
	     ? get_queue( )
	     : ensure( temporary_get( queue ) ) );
	auto tmp_fn = Q_TEMPORARILY_COPYABLE( fn );
	auto state = state_;

	auto perform = [ deferred, tmp_fn, state ]( ) mutable
	{
		tmp_fn.consume( )( );

		auto value = state->consume( );
		deferred->set_expect( std::move( value ) );
	};

	state_->signal( )->push( std::move( perform ),
	                         ensure( temporary_forward( queue ) ) );

	return deferred->get_promise( );
}

} // namespace detail

template< typename Fn >
promise< Q_RESULT_OF( Fn ) >
make_promise( Fn&& fn, const queue_ptr& queue )
{
	auto deferred = ::q::detail::defer< Q_RESULT_OF( Fn ) >::
		construct( queue );

	deferred->satisfy_by_fun( std::forward< Fn >( fn ) );

	return deferred->get_promise( );
}

} // namespace q

#endif // LIBQ_PROMISE_PROMISE_IMPL_HPP
