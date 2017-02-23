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

#ifndef LIBQ_PROMISE_PROMISE_HPP
#define LIBQ_PROMISE_PROMISE_HPP

namespace q {

namespace detail {


template< bool Shared, typename... T >
struct make_promise_type
{
	typedef ::q::promise< T... > type;
};
template< typename... T >
struct make_promise_type< true, T... >
{
	typedef ::q::shared_promise< T... > type;
};
template< bool Shared, typename... T >
using make_promise_type_t = typename make_promise_type< Shared, T... >::type;

template< typename... Args >
struct suitable_promise
{
	typedef promise< Args... > type;
};
template< >
struct suitable_promise< void >
: suitable_promise< >
{ };
template< typename... Args >
struct suitable_promise< std::tuple< Args... > >
: suitable_promise< Args... >
{ };

template< typename... Args >
using suitable_promise_t = typename suitable_promise< Args... >::type;

template< bool Shared, typename... Args >
class generic_promise
{
public:
	static_assert(
		q::all_types_are_non_references< Args... >::value,
		"Promises of references aren't allowed"
	);

	using argument_types = arguments< Args... >;

	typedef bool_type_t< Shared >                  shared_type;
	typedef std::tuple< Args... >                  tuple_type;
	typedef generic_promise< Shared, Args... >     this_type;
	typedef promise< Args... >                     unique_this_type;
	typedef shared_promise< Args... >              shared_this_type;
	typedef make_promise_type_t< Shared, Args... > promise_this_type;
	typedef promise_state< tuple_type, Shared >    state_type;
	typedef promise_state< tuple_type, false >     unique_state_type;
	typedef unique_this_type                       promise_type;
	typedef shared_this_type                       shared_promise_type;
	typedef expect< tuple_type >                   tuple_expect_type;
	typedef typename std::conditional<
		sizeof...( Args ) < 2,
		::q::expect<
			typename ::q::arguments< Args..., void >::first_type
		>,
		void
	>::type                                        short_expect_type;
	typedef typename std::conditional<
		sizeof...( Args ) < 2,
		typename ::q::arguments< Args..., void >::first_type,
		tuple_type
	>::type                                        first_or_all_types;

	template< typename... T >
	struct is_valid_arguments
	: ::q::is_argument_same_or_convertible_incl_void_t<
		argument_types,
		arguments< T... >
	>
	{ };

	template< typename... T >
	struct is_valid_arguments< arguments< T... > >
	: ::q::is_argument_same_or_convertible_incl_void_t<
		argument_types,
		arguments< T... >
	>
	{ };

	generic_promise( state_type&& state, queue_ptr queue )
	: state_( ::q::make_shared< state_type >( std::move( state ) ) )
	, queue_( std::move( queue ) )
	{ }

	generic_promise(
		std::shared_ptr< state_type > state, queue_ptr queue
	)
	: state_( std::move( state ) )
	, queue_( std::move( queue ) )
	{ }

	generic_promise( this_type&& ref ) = default;
	generic_promise( const this_type& ref ) = default;

	generic_promise& operator=( this_type&& ) = default;
	generic_promise& operator=( const this_type& ) = default;

	virtual ~generic_promise( )
	{ }

	/**
	 * @return queue_ptr The current queue for this promise
	 */
	queue_ptr get_queue( ) const noexcept
	{
		return queue_;
	}

	/**
	 * *Consumes* the promise and returns a new promise with a new default
	 * queue. The promise on which this is called, is thereby left in an
	 * undefined state and must not be used again, just like `then()`.
	 */
	template< typename Queue >
	typename std::enable_if<
		std::is_same<
			typename std::decay< Queue >::type,
			queue_ptr
		>::value,
		this_type
	>::type
	use_queue( Queue&& queue ) noexcept
	{
		queue_ = std::forward< Queue >( queue );
		return this_type( std::move( *this ) );
	}

	/**
	 * The API:
	 *
	 * Below, E is either void, std::exception, or any other type for which
	 * exceptions will be tried to be caught.
	 * A generic_promise of type T... (actually tuple<T...>) has:
	 *
	 *   * then( T, ... )        -> X... => generic_promise< X... >
	 *   * then( tuple< T... > ) -> X... => generic_promise< X... >
	 *
	 *   * fail( E ) -> void // Can not be continued (except for done())
	 *   * fail( E ) -> generic_promise< T... >
	 *                  // Can be continued, suitable for "retry" flow
	 *
	 *   * done( ) -> void
	 *
	 */

	/**
	 * ( ... ) -> value
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		is_function_t< Fn >::value
		and
		!first_argument_is_tuple< Fn >::value
		and
		this_type::template is_valid_arguments<
			arguments_of_t< Fn >
		>::value
		and
		!is_promise< result_of_t< Fn > >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		suitable_promise_t< result_of_as_tuple_t< Fn > >
	>::type
	then( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * ( std::tuple< ... > ) -> value
	 */
	template< typename Fn, typename Queue = queue_ptr >
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
		!is_promise< result_of_t< Fn > >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		suitable_promise_t< result_of_as_tuple_t< Fn > >
	>::type
	then( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * ( ... ) -> promise< value >
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		is_function_t< Fn >::value
		and
		!first_argument_is_tuple< Fn >::value
		and
		this_type::template is_valid_arguments<
			arguments_of_t< Fn >
		>::value
		and
		is_promise< result_of_t< Fn > >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		typename result_of_t< Fn >::unique_this_type
	>::type
	then( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * ( std::tuple< ... > ) -> promise< value >
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		is_function_t< Fn >::value
		and
		first_argument_is_tuple< Fn >::value
		and
		::q::is_argument_same_or_convertible_incl_void_t<
			argument_types,
			tuple_arguments_t< first_argument_of_t< Fn > >
		>::value
		and
		is_promise< result_of_t< Fn > >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		typename result_of_t< Fn >::unique_this_type
	>::type
	then( Fn&& fn, Queue&& queue = nullptr );

	template< typename Logger, typename Queue = queue_ptr >
	typename std::enable_if<
		is_same_type< Logger, log_chain_generator >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		promise_this_type
	>::type
	then( Logger&& logger, Queue&& queue = nullptr );

	/**
	 * A special case "then", where the function to be run, is wrapped in
	 * an async_task. This async_task will be run synchronously, and is
	 * expected to return immediately.
	 * Although the async_task is run synchronously, it performs some task
	 * asynchronously to eventually resolve the promise.
	 */
	template< typename AsyncTask >
	typename std::enable_if<
		is_same_type< AsyncTask, async_task >::value,
		promise_this_type
	>::type
	then( AsyncTask&& task );

	/**
	 * The different fail functions are defined by the following arguments:
	 *   * exception_ptr, matching any kind of error
	 *   * E, matching any data of type E
	 *
	 * The function can return:
	 *   * T or tuple< T... >  Any data where T (T...) is of the same type
	 *                         as the types in this promise.
	 *   * P< T... >           A promise of the same as above, awaited
	 *
	 * So, we end up with the following versions of fail():
	 *   * exception_ptr -> tuple< T... >
	 *   * exception_ptr -> P< tuple< T... > >
	 *   * E             -> tuple< T... >
	 *   * E             -> P< tuple< T... > >
	 *
	 * The return value of @c fail() is always a promise of the same type
	 * as the promise on which @c fail() is run, just like with @c finally.
	 */

	/**
	 * std::exception_ptr -> tuple< T... >
	 */
	template< typename Fn, typename Queue = queue_ptr >
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
			::template is_convertible_to< argument_types >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		promise_this_type
	>::type
	fail( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * std::exception_ptr -> P< tuple< T... > >
	 */
	template< typename Fn, typename Queue = queue_ptr >
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
			::template is_convertible_to< argument_types >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		promise_this_type
	>::type
	fail( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * E -> tuple< T... >
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		is_function_t< Fn >::value
		and
		arity_of_t< Fn >::value == 1
		and
		!arguments_of_are_t< Fn, std::exception_ptr >::value
		and
		tuple_arguments_t< result_of_t< Fn > >
			::template is_convertible_to< argument_types >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		promise_this_type
	>::type
	fail( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * E -> P< tuple< T... > >
	 */
	template< typename Fn, typename Queue = queue_ptr >
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
			::template is_convertible_to< argument_types >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		promise_this_type
	>::type
	fail( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * tap() works like then() in that it will only be called if the
	 * promise has a value, not exception.
	 * However, it works like finally() in the sense that it cannot
	 * manipulate the value (other than potentially making the returned
	 * promise contain an exception instead, if this function throws an
	 * exception (synchronously or asynchronously).
	 *
	 * This is useful for inspecting a promise without altering the chain.
	 */

	/**
	 * ( T... ) -> void
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		is_function_t< Fn >::value
		and
		this_type::template is_valid_arguments<
			arguments_of_t< Fn >
		>::value
		and
		std::is_void< result_of_t< Fn > >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		unique_this_type
	>::type
	tap( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * ( T... ) -> P< >
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		is_function_t< Fn >::value
		and
		this_type::template is_valid_arguments<
			arguments_of_t< Fn >
		>::value
		and
		is_promise< result_of_t< Fn > >::value
		and
		result_of_t< Fn >::argument_types::empty_or_voidish::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		unique_this_type
	>::type
	tap( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * ( std::tuple< ... > ) -> void
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		is_function_t< Fn >::value
		and
		first_argument_is_tuple< Fn >::value
		and
		::q::is_argument_same_or_convertible_incl_void_t<
			argument_types,
			tuple_arguments_t< first_argument_of_t< Fn > >
		>::value
		and
		std::is_void< result_of_t< Fn > >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		unique_this_type
	>::type
	tap( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * ( std::tuple< ... > ) -> P< >
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		is_function_t< Fn >::value
		and
		first_argument_is_tuple< Fn >::value
		and
		::q::is_argument_same_or_convertible_incl_void_t<
			argument_types,
			tuple_arguments_t< first_argument_of_t< Fn > >
		>::value
		and
		is_promise< result_of_t< Fn > >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		unique_this_type
	>::type
	tap( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * tap_error() works like tap(), except that it is only called if the
	 * promsie contains an exception and not a value. Like tap(), it won't
	 * alter the chain, and the provided functions can therefore only
	 * return void or an empty promise.
	 */

	/**
	 * std::exception_ptr -> void
	 */
	template< typename Fn, typename Queue = queue_ptr >
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
		std::is_void< result_of_t< Fn > >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		unique_this_type
	>::type
	tap_error( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * std::exception_ptr -> P< >
	 */
	template< typename Fn, typename Queue = queue_ptr >
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
		result_of_t< Fn >::argument_types::empty_or_voidish::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		unique_this_type
	>::type
	tap_error( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * E -> void
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		is_function_t< Fn >::value
		and
		arity_of_t< Fn >::value == 1
		and
		!arguments_of_are_t< Fn, std::exception_ptr >::value
		and
		std::is_void< result_of_t< Fn > >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		unique_this_type
	>::type
	tap_error( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * E -> P< >
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		is_function_t< Fn >::value
		and
		arity_of_t< Fn >::value == 1
		and
		!arguments_of_are_t< Fn, std::exception_ptr >::value
		and
		is_promise< result_of_t< Fn > >::value
		and
		result_of_t< Fn >::argument_types::empty_or_voidish::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		unique_this_type
	>::type
	tap_error( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * A finally() function is always run, ignoring the current state, i.e.
	 * if the previous task threw an exception or returned a value.
	 *
	 * A normal use for finally() is to clean up.
	 */

	/**
	 * void -> void
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		is_function_t< Fn >::value
		and
		std::is_void< result_of_t< Fn > >::value
		and
		arity_of_t< Fn >::value == 0
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		promise_this_type
	>::type
	finally( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * void -> P< >
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		is_function_t< Fn >::value
		and
		::q::is_promise< result_of_t< Fn > >::value
		and
		result_of_t< Fn >::argument_types::empty::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		promise_this_type
	>::type
	finally( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * When the promise is resolved, execution is stopped for a certain
	 * duration, and continues (resolves) when this duration has passed,
	 * forwarding the value to the resulting promise.
	 * The backend timer used, will be the one implicitly bound to the
	 * current queue (or by the options provided queue).
	 */
	promise_this_type
	delay(
		timer::duration_type duration,
		queue_options options = queue_options( )
	);

	/**
	 * Converts this promise of tuple of T (or T...) into a promise of
	 * @c q::expect of tuple of T (or T...).
	 *
	 * The resulting promise will not fail, and will always resolve to its
	 * inner q::expect object. This expect object contains either the value
	 * or the exception immediately ("synchronously").
	 *
	 * Technically:
	 *   promise< T... > -> promise< expect< tuple< T... > > >
	 *
	 * This is useful e.g. to collect a set of promises where some might
	 * have been resolved and some rejected. The reflected value, i.e. the
	 * expect-wrapped value, is always successful, so the set of promises
	 * can be inspected for success or failure individually.
	 */
	q::promise< tuple_expect_type > reflect_tuple( );

	/**
	 * Same as reflect_tuple( ), except this will shortcut the inner tuple
	 * of the @c expect if the size of T... is less than 2, i.e. when this
	 * promise is of either nothing or just one type, the result promise
	 * will be of an expect of T (or nothing) itself.
	 *
	 * Technically:
	 *
	 *   For a promise< > or promise< T >:
	 *     promise< T/nothing > -> promise< expect< T/nothing > >
	 *
	 *   For a promise< T1, T2... >:
	 *     Same as reflect_tuple( )
	 */
	template< bool Simplified = ( sizeof...( Args ) < 2 ) >
	typename std::enable_if<
		Simplified,
		q::promise< short_expect_type >
	>::type
	reflect( );

	template< bool Simplified = ( sizeof...( Args ) < 2 ) >
	typename std::enable_if<
		!Simplified,
		q::promise< tuple_expect_type >
	>::type
	reflect( )
	{
		return reflect_tuple( );
	}

	template< typename... U, typename _V = void >
	typename std::enable_if<
		std::is_void< _V >::value
		and
		argument_types::empty::value,
		promise< U... >
	>::type
	forward( U&&... values );

	void done( )
	{
		// TODO: Implement
		// This function should trigger the "uncaught exception
		// handler" and terminate the application.
		// The default action for promises which fail but where the
		// exception is never handled is to run the "uncaught exception
		// handler", but not necessarily terminate the applcation.
		//
		// Note, uncaught exceptions are actually not triggered at all
		// right now, this is yet to be implemented.
	}

private:
	friend class ::q::promise< Args... >;
	friend class ::q::shared_promise< Args... >;

	template< typename Queue >
	typename std::enable_if<
		std::is_same<
			typename std::decay< Queue >::type,
			queue_ptr
		>::value,
		queue_ptr
	>::type
	ensure( Queue&& queue )
	{
		if ( queue )
			return std::forward< Queue >( queue );
		return queue_;
	}

	std::shared_ptr< state_type > state_;
	queue_ptr queue_;
};

} // namespace detail

template< typename... T >
class promise
: public detail::generic_promise< false, T... >
{
	typedef promise< T... >                        this_type;
	typedef detail::generic_promise< false, T... > base_type;

public:
	typedef ::q::is_copy_constructible< T... > shareable;

	promise(
		typename base_type::state_type&& state, const queue_ptr& queue
	)
	: base_type( std::move( state ), queue )
	{ }

	promise( ) = delete;
	promise( this_type&& ) = default;
	promise( const this_type& ) = delete;
	promise& operator=( this_type&& ) = default;
	promise& operator=( const this_type& ) = delete;

	promise( detail::generic_promise< false, T... >&& ref )
	: base_type( std::move( ref.state_ ), ref.queue_ )
	{ }

	template< bool B = shareable::value >
	typename std::enable_if<
		B,
		shared_promise< T... >
	>::type
	share( )
	{
		return shared_promise< T... >(
			base_type::state_->acquire( ), this->get_queue( ) );
	}

	/**
	 * Throws away the value this promise holds, and returns an empty
	 * promise.
	 */
	::q::promise< > strip( );
};

template< typename... T >
class shared_promise
: public detail::generic_promise< true, T... >
{
	typedef shared_promise< T... >                this_type;
	typedef detail::generic_promise< true, T... > base_type;

public:
	shared_promise( typename base_type::state_type&& state,
	                const queue_ptr& queue )
	: base_type( std::move( state ), queue )
	{ }

	shared_promise(
		detail::promise_state_data< std::tuple< T... >, false >&& state,
		const queue_ptr& queue )
	: base_type( std::move( state ), queue )
	{ }

	shared_promise( detail::generic_promise< true, T... >&& ref )
	: base_type( std::move( ref.state_ ), ref.queue_ )
	{ }

	shared_promise( ) = delete;
	shared_promise( this_type&& ) = default;
	shared_promise( const this_type& ) = default;
	shared_promise& operator=( this_type&& ) = default;
	shared_promise& operator=( const this_type& ) = default;

	promise< T... > unshare( ) noexcept // TODO: analyze noexcept here
	{
		return this->then( [ ]( typename base_type::tuple_type&& value )
		{
			return value;
		} );
	}

	/**
	 * Throws away the value this shared_promise holds, and returns an
	 * empty shared_promise.
	 */
	::q::shared_promise< > strip( );
};

} // namespace q

#endif // LIBQ_PROMISE_PROMISE_HPP
