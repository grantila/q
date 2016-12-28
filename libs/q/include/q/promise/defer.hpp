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

#ifndef LIBQ_PROMISE_DEFER_HPP
#define LIBQ_PROMISE_DEFER_HPP

namespace q {

namespace detail {

template< typename... Args >
struct suitable_defer
{
	typedef defer< Args... > type;
};
template< >
struct suitable_defer< void >
: suitable_defer< >
{ };
template< typename... Args >
struct suitable_defer< std::tuple< Args... > >
: suitable_defer< Args... >
{ };

template< typename... Args >
using suitable_defer_t = typename suitable_defer< Args... >::type;

template< typename... T >
class defer
: public std::enable_shared_from_this< defer< T... > >
{
public:
	typedef ::q::arguments< T... >                          arguments_type;
	typedef std::tuple< T... >                              tuple_type;
	typedef expect< tuple_type >                            expect_type;
	typedef detail::promise_state_data< tuple_type, false > state_data_type;
	typedef detail::promise_state< tuple_type, false >      state_type;
	typedef promise< T... >                                 promise_type;
	typedef shared_promise< T... >                          shared_promise_type;

	void set_expect( expect_type&& exp )
	{
		if ( exp.has_exception( ) )
			set_exception( exp.exception( ) );
		else
			set_value( exp.consume( ) );
	}

	/**
	 * Sets the expected value based on an expect< tuple< expect< > > >
	 */
	void set_inner_expect( ::q::expect< std::tuple< > >&& exp )
	{
		if ( exp.has_exception( ) )
		{
			set_value(
				::q::refuse< void >( exp.exception( ) ) );
		}
		else
		{
			set_value( ::q::fulfill< void >( ) );
		}
	}

	/**
	 * Sets the expected value based on an expect< tuple< expect< _T > > >
	 */
	template< typename _T >
	typename std::enable_if<
		sizeof...( T ) == 1
		and
		::q::is_argument_same_or_convertible_t<
			::q::arguments< _T >,
			arguments_type
		>::value
	>::type
	set_inner_expect( ::q::expect< std::tuple< _T > >&& exp )
	{
		if ( exp.has_exception( ) )
		{
			set_value( ::q::refuse< _T >( exp.exception( ) ) );
		}
		else
		{
			auto tuple = exp.consume( );
			auto inner_value = std::move( std::get< 0 >( tuple ) );
			set_value( ::q::fulfill< _T >(
				std::move( inner_value ) ) );
		}
	}

	inline void set_value( tuple_type&& tuple )
	{
		auto value = ::q::fulfill< tuple_type >( std::move( tuple ) );
		promise_.set_value( std::move( value ) );
		signal_->done( );
	}

	inline void set_value( const tuple_type& tuple )
	{
		auto value = ::q::fulfill< tuple_type >( tuple_type( tuple ) );
		promise_.set_value( std::move( value ) );
		signal_->done( );
	}

	template< typename... Args >
	typename std::enable_if<
		(
			q::arguments< Args... >::size::value != 1
			or
			!q::is_tuple<
				typename std::decay< Args >::type...
			>::value
		)
		and
		::q::is_argument_same_or_convertible_t<
			q::arguments< Args... >,
			q::arguments< T... >
		>::value
	>::type
	set_value( Args&&... args )
	{
		set_value( std::make_tuple( std::forward< Args >( args )... ) );
	}

	/**
	 * Sets an exception to this defer, the exception can be any kind of
	 * value, but should preferably be either a @c q::exception or a
	 * @c std::exception_ptr.
	 */
	template< typename E >
	typename std::enable_if<
		!std::is_same<
			typename q::remove_cv_ref< E >::type,
			std::exception_ptr
		>::value
	>::type
	set_exception( E&& e )
	{
		set_exception(
			std::make_exception_ptr( std::forward< E >( e ) )
		);
	}

	void set_exception( const std::exception_ptr& e )
	{
		promise_.set_value( ::q::refuse< tuple_type >( e ) );
		signal_->done( );
	}

	void set_current_exception( )
	{
		set_exception( std::current_exception( ) );
	}

	/**
	 * fn( args... ), set_value( void )
	 */
	template< typename Fn, typename... Args >
	typename std::enable_if<
		sizeof...( T ) == 0
		and
		Q_RESULT_OF_AS_ARGUMENT( Fn )::empty_or_voidish::value
		and
		Q_ARGUMENTS_ARE_CONVERTIBLE_FROM_INCL_VOID( Fn, Args... )::value
	>::type
	set_by_fun( Fn&& fn, Args&&... args )
	{
		try
		{
			::q::call_with_args(
				std::forward< Fn >( fn ),
				std::forward< Args >( args )...
			);
			set_value( std::tuple< >( ) );
		}
		catch ( ... )
		{
			set_current_exception( );
		}
	}

	/**
	 * set_value( fn( args... ) )
	 */
	template< typename Fn, typename... Args >
	typename std::enable_if<
		( sizeof...( T ) > 0 )
		and
		::q::result_of_as_argument_t< Fn >::template same<
			::q::arguments< T... >
		>::value
		and
		Q_ARGUMENTS_ARE_CONVERTIBLE_FROM_INCL_VOID( Fn, Args... )::value
	>::type
	set_by_fun( Fn&& fn, Args&&... args )
	{
		try
		{
			set_value(
				::q::call_with_args(
					std::forward< Fn >( fn ),
					std::forward< Args >( args )...
				)
			);
		}
		catch ( ... )
		{
			set_current_exception( );
		}
	}

	/**
	 * fn( tuple< args >... ), set_value( void )
	 */
	template< typename Fn, typename Args >
	typename std::enable_if<
		sizeof...( T ) == 0
		and
		Q_RESULT_OF_AS_ARGUMENT( Fn )::empty_or_voidish::value
		and
		::q::is_tuple< Args >::value
		and
		tuple_arguments_t< Args >
			::template is_convertible_to<
				arguments_of_t< Fn >
			>::value
	>::type
	set_by_fun( Fn&& fn, Args&& args )
	{
		try
		{
			::q::call_with_args_by_tuple(
				std::forward< Fn >( fn ),
				std::forward< Args >( args )
			);
			set_value( std::tuple< >( ) );
		}
		catch ( ... )
		{
			set_current_exception( );
		}
	}

	/**
	 * set_value( fn( tuple< args >... ) )
	 */
	template< typename Fn, typename Args >
	typename std::enable_if<
		( sizeof...( T ) > 0 )
		and
		::q::result_of_as_argument_t< Fn >::template same<
			::q::arguments< T... >
		>::value
		and
		::q::is_tuple< Args >::value
		and
		tuple_arguments_t< Args >
			::template is_convertible_to_incl_void<
				::q::arguments_of_t< Fn >
			>::value
	>::type
	set_by_fun( Fn&& fn, Args&& args )
	{
		try
		{
			set_value(
				::q::call_with_args_by_tuple(
					std::forward< Fn >( fn ),
					std::forward< Args >( args )
				)
			);
		}
		catch ( ... )
		{
			set_current_exception( );
		}
	}

	/**
	 * Sets the promise (i.e. value or exception) by a function.
	 *
	 * inner promise = fn( args... )
	 */
	template< typename Fn, typename... Args >
	typename std::enable_if<
		Q_ARITY_OF( Fn ) == sizeof...( Args )
		and
		(
			Q_ARITY_OF( Fn ) != 1
			or
			Q_ARGUMENTS_ARE_CONVERTIBLE_FROM_INCL_VOID(
				Fn, Args...
			)::value
		)
		and
		::q::is_promise< Q_RESULT_OF( Fn ) >::value
	>::type
	set_by_fun( Fn&& fn, Args&&... args )
	{
		try
		{
			satisfy(
				::q::call_with_args(
					std::forward< Fn >( fn ),
					std::forward< Args >( args )...
				)
			);
		}
		catch ( ... )
		{
			set_current_exception( );
		}
	}

	/**
	 * Sets the promise (i.e. value or exception) by a function.
	 *
	 * inner promise = fn( ), { Args == std::tuple< > }
	 */
	template< typename Fn, typename... Args >
	typename std::enable_if<
		arguments<
			typename std::decay< Args >::type...
		>::template same< q::arguments< std::tuple< > > >::value
		and
		Q_ARITY_OF( Fn ) == 0
		and
		::q::is_promise< Q_RESULT_OF( Fn ) >::value
	>::type
	set_by_fun( Fn&& fn, Args&&... args )
	{
		try
		{
			satisfy( fn( ) );
		}
		catch ( ... )
		{
			set_current_exception( );
		}
	}

	/**
	 * inner promise = fn( tuple< args >... )
	 */
	template< typename Fn, typename Args >
	typename std::enable_if<
		is_tuple< Args >::value
		and
		tuple_arguments_t<
			typename std::decay< Args >::type
		>::template is_convertible_to_incl_void<
			arguments_of_t< Fn >
		>::value
		and
		(
			tuple_arguments_t<
				typename std::decay< Args >::type
			>::template same< arguments< void_t > >::value
			or
			!std::is_same<
				typename std::decay<
					first_argument_of_t< Fn >
				>::type,
				void_t
			>::value
		)
		and
		::q::is_promise< Q_RESULT_OF( Fn ) >::value
	>::type
	set_by_fun( Fn&& fn, Args&& args )
	{
		try
		{
			satisfy(
				::q::call_with_args_by_tuple(
					std::forward< Fn >( fn ),
					std::forward< Args >( args )
				)
			);
		}
		catch ( ... )
		{
			set_current_exception( );
		}
	}

	/**
	 * ( tuple< > )      -> fn( tuple< void_t > )
	 * ( tuple< void > ) -> fn( tuple< void_t > )
	 */
	template< typename Fn, typename Args >
	typename std::enable_if<
		is_tuple< Args >::value
		and
		tuple_arguments_t<
			typename std::decay< Args >::type
		>::empty_or_void::value
		and
		arguments_of_t< Fn >::size::value == 1
		and
		(
			std::is_same<
				typename std::decay<
					first_argument_of_t< Fn >
				>::type,
				std::tuple< void_t >
			>::value
			or
			std::is_same<
				typename std::decay<
					first_argument_of_t< Fn >
				>::type,
				std::tuple< void_t&& >
			>::value
		)
	>::type
	set_by_fun( Fn&& fn, Args&& args )
	{
		set_by_fun(
			std::forward< Fn >( fn ),
			std::make_tuple( void_t( ) )
		);
	}

	/**
	 * ( tuple< > )      -> fn( void_t )
	 * ( tuple< void > ) -> fn( void_t )
	 */
	template< typename Fn, typename Args >
	typename std::enable_if<
		is_tuple< typename std::decay< Args >::type >::value
		and
		tuple_arguments_t<
			typename std::decay< Args >::type
		>::empty_or_void::value
		and
		arguments_of_t< Fn >::size::value == 1
		and
		std::is_same<
			typename std::decay< first_argument_of_t< Fn > >::type,
			void_t
		>::value
	>::type
	set_by_fun( Fn&& fn, Args&& args )
	{
		set_by_fun( std::forward< Fn >( fn ), void_t( ) );
	}

	/**
	 * ( tuple< void > )   -> fn( )
	 * ( tuple< void_t > ) -> fn( )
	 */
	template< typename Fn, typename Args >
	typename std::enable_if<
		is_tuple< typename std::decay< Args >::type >::value
		and
		std::tuple_size<
			typename std::decay< Args >::type
		>::value == 1
		and
		tuple_arguments_t<
			typename std::decay< Args >::type
		>::empty_or_voidish::value
		and
		arguments_of_t< Fn >::empty::value
	>::type
	set_by_fun( Fn&& fn, Args&& args )
	{
		set_by_fun( std::forward< Fn >( fn ) );
	}

	void satisfy( promise_type&& promise )
	{
		auto _this = this->shared_from_this( );

		promise
		.then( [ _this ]( tuple_type&& tuple )
		{
			_this->set_value( std::move( tuple ) );
		} )
		.fail( [ _this ]( std::exception_ptr&& e )
		{
			_this->set_exception( std::move( e ) );
		} );
	}

	void satisfy( shared_promise_type promise )
	{
		auto _this = this->shared_from_this( );

		promise
		.then( [ _this ]( const tuple_type& tuple )
		{
			_this->set_value( tuple );
		} )
		.fail( [ _this ]( std::exception_ptr&& e )
		{
			_this->set_exception( std::move( e ) );
		} );
	}

	/*
	 void satisfy( const shared_promise_type& promise )
	 {
	 promise_ = std::move( promise.privatize( ) );
	 signal_->done( );
	 }
	 */

	// Moves the promise out of the defer
	promise_type get_promise( )
	{
		return std::move( deferred_ );
	}

	template< typename Promise >
	typename std::enable_if<
		Promise::shared_type::value
		and
		std::is_same<
			tuple_type,
			typename Promise::tuple_type
		>::value,
		Promise
	>::type
	get_suitable_promise( )
	{
		return get_promise( ).share( );
	}

	template< typename Promise >
	typename std::enable_if<
		!Promise::shared_type::value
		and
		std::is_same<
			tuple_type,
			typename Promise::tuple_type
		>::value,
		Promise
	>::type
	get_suitable_promise( )
	{
		return get_promise( );
	}

	queue_ptr get_queue( ) const
	{
		return queue_;
	}

	static std::shared_ptr< defer< T... > >
	construct( const queue_ptr& queue )
	{
		typedef typename state_data_type::future_type future_type;

		std::promise< expect_type > std_promise;
		future_type future = std_promise.get_future( );

		state_data_type state_data( std::move( future ) );
		state_type state( std::move( state_data ) );

		auto signal = state.signal( );

		promise_type q_promise( std::move( state ), queue );

		return ::q::make_shared_using_constructor< defer< T... > >(
			std::move( std_promise ),
			std::move( signal ),
			std::move( q_promise ) );
	}

protected:
	defer( ) = delete;

	defer( std::promise< expect_type >&& promise,
	       promise_signal_ptr&& signal,
	       promise_type&& deferred )
	: promise_( std::move( promise ) )
	, signal_( std::move( signal ) )
	, deferred_( std::move( deferred ) )
	, queue_( deferred_.get_queue( ) )
	{ }

private:
	std::promise< expect_type > promise_;
	promise_signal_ptr          signal_;

	promise_type                deferred_;

	queue_ptr                   queue_;
};

template< typename... T >
class defer< std::tuple< T... > >
: public defer< T... >
{ };

} // namespace detail

} // namespace q

#endif // LIBQ_PROMISE_DEFER_HPP
