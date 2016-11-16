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

#ifndef LIBQ_RX_OBSERVABLE_OBSERVABLE_HPP
#define LIBQ_RX_OBSERVABLE_OBSERVABLE_HPP

namespace q { namespace rx {

namespace detail {

template< typename T >
struct observable_types
{
	typedef q::expect< T > expect_type;
	typedef q::channel< T > channel_type;
	typedef q::readable< T > readable_type;
	typedef q::writable< T > writable_type;
	typedef std::tuple< T > tuple_type;
};

template< typename... T >
struct observable_types< std::tuple< T... > >
{
	typedef q::expect< std::tuple< T... > > expect_type;
	typedef q::channel< T... > channel_type;
	typedef q::readable< T... > readable_type;
	typedef q::writable< T... > writable_type;
	typedef std::tuple< T... > tuple_type;
};

template< >
struct observable_types< void >
{
	typedef q::expect< void > expect_type;
	typedef q::channel< > channel_type;
	typedef q::readable< > readable_type;
	typedef q::writable< > writable_type;
	typedef std::tuple< > tuple_type;
};

} // namespace detail

template< typename T = void >
class observable
: detail::observable_types< T >
{
	typedef detail::observable_types< T > base_types;
	typedef typename base_types::expect_type expect_type;
	typedef typename base_types::channel_type channel_type;
	typedef typename base_types::readable_type readable_type;
	typedef typename base_types::writable_type writable_type;
	typedef typename base_types::tuple_type tuple_type;
	typedef typename q::objectify< T >::type void_safe_type;

public:
	typedef T element_type;
	typedef observable< T > this_type;

	/**********************************************************************
	 * Creators
	 *********************************************************************/

	/************************************************
	 * Creators: default constructor
	 ***********************************************/

	observable( )
	{ }

	/************************************************
	 * Creators: from q::channel
	 ***********************************************/

	observable( readable_type&& readable )
	: readable_( std::make_shared<
		detail::observable_readable_direct< T >
	>( std::move( readable ) ) )
	{ }

	observable( channel_type channel )
	: observable( channel.get_readable( ) )
	{ }

	observable( q::readable< expect_type >&& readable )
	: readable_( std::make_shared<
		detail::observable_readable_expect< T >
	>( std::move( readable ) ) )
	{ }

	observable( q::channel< expect_type > channel )
	: observable( channel.get_readable( ) )
	{ }

	observable( q::readable< q::promise< tuple_type > >&& readable )
	: readable_( std::make_shared<
		detail::observable_readable_promise< T >
	>( std::move( readable ) ) )
	{ }

	observable( q::channel< q::promise< tuple_type > > channel )
	: observable( channel.get_readable( ) )
	{ }

	observable( q::readable< q::shared_promise< tuple_type > >&& readable )
	: readable_( std::make_shared<
		detail::observable_readable_shared_promise< T >
	>( std::move( readable ) ) )
	{ }

	observable( q::channel< q::shared_promise< tuple_type > > channel )
	: observable( channel.get_readable( ) )
	{ }

	/************************************************
	 * Creators: Empty
	 ***********************************************/

	static observable< T > empty( const q::queue_ptr& queue );

	/************************************************
	 * Creators: Never
	 ***********************************************/

	static observable< T > never( const q::queue_ptr& queue );

	/************************************************
	 * Creators: Error ("Throw" in ReactiveX)
	 ***********************************************/

	static observable< T > error(
		const q::queue_ptr& queue, std::exception_ptr exception );

	template< typename Exception >
	static typename std::enable_if<
		!std::is_same<
			typename std::decay< Exception >::type,
			std::exception_ptr
		>::value,
		observable< T >
	>::type error( const q::queue_ptr& queue, Exception&& exception );

	/************************************************
	 * Creators: Create
	 ***********************************************/

	class observer
	{
	public:
		observer( ) = default;
		observer( observer&& ) = default;
		observer( const observer& ) = default;
		observer( q::writable< T > writable );

		void on_next( T&& t );
		void on_next( const T& t );
		void on_completed( );
		template< typename Error >
		void on_error( Error&& error );

	private:
		q::writable< T > writable_;
	};

	static observable< T > create(
		std::function< void( observer ) > fn,
		create_options options
	);

	/************************************************
	 * Creators: Defer
	 ***********************************************/

	// TODO: Consider subscribing of observables, potentially with another
	//       class, subscribable_observable.
	//       In q-rx, all observables buffer data, so the concept of
	//       subscribing doesn't apply the same way.

	/************************************************
	 * Creators: From
	 ***********************************************/

	static observable< T > from( q::channel< T > channel );

	template< typename U, typename Queue >
	static typename std::enable_if<
		q::is_container_v< std::decay_t< U > >
		and
		std::is_same<
			objectify_t< typename std::decay_t< U >::value_type >,
			objectify_t< T >
		>::value,
		observable< T >
	>::type
	from( U&& container, Queue&& queue );

	template< typename Queue, typename Tag, typename U = T >
	static typename std::enable_if<
		!std::is_void< U >::value,
		observable< T >
	>::type
	from(
		std::iterator< Tag, U > begin,
		std::iterator< Tag, U > end,
		Queue&& queue
	);

	/************************************************
	 * Creators: Interval
	 ***********************************************/

	// TODO: Implement (requires an io-dispatcher, or at least timer
	//       dispatch support)

	/************************************************
	 * Creators: Just
	 ***********************************************/

	template< typename... U >
	static typename std::enable_if<
		q::are_all_same<
			T,
			typename std::decay< U >::type...
		>::value,
		observable< T >
	>::type
	just( const queue_ptr& queue, U&&... values );

	/************************************************
	 * Creators: Range
	 ***********************************************/

	/**
	 * This range allows any kind of T that can be advanced with the
	 * postfix ++ operator. In other words, this works with numeric data
	 * types just as well as iterators.
	 */
	template< typename U = T >
	static typename std::enable_if<
		!std::is_void< U >::value
		and
		!std::is_same< void_t, typename std::decay< U >::type >::value,
		observable< T >
	>::type
	range( U&& start, std::size_t count, create_options options );

	template< typename U = T >
	static typename std::enable_if<
		std::is_void< U >::value,
		observable< T >
	>::type
	range( std::size_t count, create_options options );

	template< typename V, typename U = T >
	static typename std::enable_if<
		std::is_void< U >::value
		and
		std::is_same< void_t, typename std::decay< V >::type >::value,
		observable< T >
	>::type
	range( V&& start, std::size_t count, create_options options );

	/************************************************
	 * Creators: Repeat
	 * This is a "create" operator according to the
	 * ReactiveX spec, but is really a "combining"
	 * operator as it operates in existing
	 * observables.
	 ***********************************************/

	/**
	 * Inifinite repetition of upstream observable, stopping only on closed
	 * downstream observable
	 */
	observable< T > repeat( combine_options options = combine_options( ) );

	/**
	 * A limit of zero equals infinite repetition (although stopping on
	 * closed downstream observable)
	 */
	observable< T > repeat(
		std::size_t limit,
		combine_options options = combine_options( )
	);

	/************************************************
	 * Creators: Start
	 ***********************************************/

	template< typename Fn >
	static typename std::enable_if<
		!q::is_promise< q::result_of_t< Fn > >::value
		and
		std::is_same< q::result_of_t< Fn >, T >::value,
		observable< T >
	>::type
	start( Fn&& fn, queue_options options );

	template< typename Fn >
	static typename std::enable_if<
		q::is_promise< q::result_of_t< Fn > >::value
		and
		q::result_of_t< Fn >::argument_types
		::template is_convertible_to< q::arguments< T > >::value,
		observable< T >
	>::type
	start( Fn&& fn, queue_options options );

	/************************************************
	 * Creators: Timer
	 ***********************************************/


	/**********************************************************************
	 * Transformers
	 *********************************************************************/

	/************************************************
	 * Transformers: Buffer
	 ***********************************************/

	/**
	 * count
	 */
	observable< std::vector< void_safe_type > >
	buffer(
		std::size_t count,
		combine_options options = combine_options( )
	);

	/**
	 * count, stride
	 *
	 * NOTE; stride is called "skip" in ReactiveX, which is an erroneous
	 * naming, since it's not how many items to skip, but the distances
	 * between the beginnings of each outgoing chunk as counted by the
	 * incoming items.
	 */
	observable< std::vector< void_safe_type > >
	buffer(
		std::size_t count,
		std::size_t stride,
		combine_options options = combine_options( )
	);

	/**
	 * closing_observable
	 */
	observable< std::vector< void_safe_type > >
	buffer(
		observable< void > closing_observable,
		combine_options options = combine_options( )
	);

	/**
	 * duration
	 */
	observable< std::vector< void_safe_type > >
	buffer(
		q::timer::duration_type duration,
		combine_options options = combine_options( )
	);

	/**
	 * duration, shift
	 */
	observable< std::vector< void_safe_type > >
	buffer(
		q::timer::duration_type duration,
		q::timer::duration_type shift,
		combine_options options = combine_options( )
	);

	/**
	 * duration, count
	 */
	observable< std::vector< void_safe_type > >
	buffer(
		q::timer::duration_type duration,
		std::size_t count,
		combine_options options = combine_options( )
	);

	/************************************************
	 * Transformers: GroupBy
	 ***********************************************/

	/**
	 * group_by of hashed key from synchronous operand.
	 */
	template< typename Operator = fake_operator, typename Fn >
	std::enable_if_t<
		should_hash_v<
			std::decay_t< q::result_of_t< Fn > >,
			Operator
		>
		and
		!q::is_promise< q::result_of_t< Fn > >::value
		and
		!q::result_of_as_argument_t< Fn >::empty_or_voidish::value,
		observable< std::tuple<
			std::decay_t< q::result_of_t< Fn > >,
			observable< T >
		> >
	>
	group_by( Fn&& fn, work_options options = work_options( ) );

	/**
	 * group_by of comparable key from synchronous operand.
	 */
	template< typename Operator = fake_operator, typename Fn >
	std::enable_if_t<
		!should_hash_v<
			std::decay_t< q::result_of_t< Fn > >,
			Operator
		>
		and
		!q::is_promise< q::result_of_t< Fn > >::value
		and
		!q::result_of_as_argument_t< Fn >::empty_or_voidish::value,
		observable< std::tuple<
			std::decay_t< q::result_of_t< Fn > >,
			observable< T >
		> >
	>
	group_by( Fn&& fn, work_options options = work_options( ) );

	/**
	 * group_by of hashed key from asynchronous operand.
	 */
	template< typename Operator = fake_operator, typename Fn >
	std::enable_if_t<
		should_hash_v<
			std::decay_t<
				typename q::result_of_t< Fn >
					::first_or_all_types
			>,
			Operator
		>
		and
		q::is_promise< q::result_of_t< Fn > >::value
		and
		!q::result_of_t< Fn >::argument_types::empty_or_voidish::value,
		observable< std::tuple<
			std::decay_t<
				typename q::result_of_t< Fn >
					::first_or_all_types
			>,
			observable< T >
		> >
	>
	group_by( Fn&& fn, work_options options = work_options( ) );

	/**
	 * group_by of comparable key from asynchronous operand.
	 */
	template< typename Operator = fake_operator, typename Fn >
	std::enable_if_t<
		!should_hash_v<
			std::decay_t<
				typename q::result_of_t< Fn >
					::first_or_all_types
			>,
			Operator
		>
		and
		q::is_promise< q::result_of_t< Fn > >::value
		and
		!q::result_of_t< Fn >::argument_types::empty_or_voidish::value,
		observable< std::tuple<
			std::decay_t<
				typename q::result_of_t< Fn >
					::first_or_all_types
			>,
			observable< T >
		> >
	>
	group_by( Fn&& fn, work_options options = work_options( ) );

	/************************************************
	 * Transformers: Map
	 ***********************************************/

	/**
	 * ( In ) -> Out
	 */
	template< typename Fn >
	typename std::enable_if<
		q::arguments_of_are_convertible_from_incl_void< Fn, T >
		and
		!q::is_promise< Q_RESULT_OF( Fn ) >::value,
		observable< Q_RESULT_OF( Fn ) >
	>::type
	map( Fn&& fn, base_options options = base_options( ) );

	/**
	 * ( In ) -> promise< Out >
	 */
	template< typename Fn >
	typename std::enable_if<
		q::arguments_of_are_convertible_from_incl_void< Fn, T >
		and
		q::is_promise< Q_RESULT_OF( Fn ) >::value,
		typename detail::tuple_to_observable<
			typename ::q::result_of< Fn >::tuple_type
		>::type
	>::type
	map( Fn&& fn, base_options options = base_options( ) );


	/**********************************************************************
	 * Consumers
	 *
	 * These are used instead of the ReactiveX default consumers (onNext,
	 * onError, onComplete) to form a nicer interface.
	 *********************************************************************/

	/***********************************************
	 * Consumers: Consume (not in ReactiveX)
	 ***********************************************/

	/**
	 * ( T [...] ) -> void
	 *
	 * Packed (in a tuple) or unpacked synchronous consumer
	 */
	template< typename Fn >
	typename std::enable_if<
		(
			(
				arguments_of_are_convertible_from_incl_void_v<
					Fn, T
				>
				and
				q::arity_of_v< Fn > != 0
			)
			or
			(
				is_tuple_v< T >
				and
				tuple_arguments_t< T >
					::template is_convertible_to_incl_void<
						arguments_of_t< Fn >
					>::value
			)
		)
		and
		std::is_void< ::q::result_of_t< Fn > >::value,
		::q::promise< std::tuple< > >
	>::type
	consume( Fn&& fn, base_options options = base_options( ) );

	/**
	 * ( T [...] ) -> promise< >
	 *
	 * Packed (in a tuple) or unpacked asynchronous consumer
	 *
	 * This will make q-rx await the returned promise, and continue only
	 * when the promise is resolved. It causes proper back pressure
	 * handling and will also stop if the promise is rejected.
	 */
	template< typename Fn >
	typename std::enable_if<
		(
			(
				arguments_of_are_convertible_from_incl_void_v<
					Fn, T
				>
				and
				q::arity_of_v< Fn > != 0
			)
			or
			(
				is_tuple_v< T >
				and
				tuple_arguments_t< T >
					::template is_convertible_to_incl_void<
						arguments_of_t< Fn >
					>::value
			)
		)
		and
		::q::is_promise_v< std::decay_t< ::q::result_of_t< Fn > > >
		and
		::q::result_of_t< Fn >::argument_types::empty_v,
		::q::promise< std::tuple< > >
	>::type
	consume( Fn&& fn, base_options options = base_options( ) );

	/**
	 * ( ) -> *
	 * No argument is wrapped to a function taking void_t
	 */
	template< typename Fn >
	typename std::enable_if<
		q::arity_of_v< Fn > == 0
		and
		std::is_same< objectify_t< T >, void_t >::value,
		::q::promise< std::tuple< > >
	>::type
	consume( Fn&& fn, base_options options = base_options( ) );

	// TODO: Implemement properly, e.g. with promises
	T onNext( );
	std::exception_ptr onError( );
	void onComplete( );

private:
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
		return readable_->get_queue( );
	}

	std::shared_ptr< detail::observable_readable< T > > readable_;
};

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_OBSERVABLE_HPP
