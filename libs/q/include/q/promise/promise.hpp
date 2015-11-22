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

template< bool Shared, typename... Args >
class generic_promise< Shared, std::tuple< Args... > >
{
public:
	typedef bool_type< Shared >                        shared_type;
	typedef arguments< Args... >                       argument_types;
	typedef std::tuple< Args... >                      tuple_type;
	typedef generic_promise< Shared, tuple_type >      this_type;
	typedef generic_promise< false, tuple_type >       unique_this_type;
	typedef generic_promise< true, tuple_type >        shared_this_type;
	typedef promise_state< tuple_type, Shared >        state_type;
	typedef promise_state< tuple_type, false >         unique_state_type;
	typedef unique_this_type                           promise_type;
	typedef shared_this_type                           shared_promise_type;

	template< typename... T >
	struct is_valid_arguments
	: ::q::is_argument_same_or_convertible< arguments< T... >, argument_types >
	{ };

	template< typename... T >
	struct is_valid_arguments< arguments< T... > >
	: ::q::is_argument_same_or_convertible< arguments< T... >, argument_types >
	{ };

	generic_promise( state_type&& state, const queue_ptr& queue )
	: state_( ::q::make_shared< state_type >( std::move( state ) ) )
	, queue_( queue )
	{ }

	generic_promise( std::shared_ptr< state_type > state, const queue_ptr& queue )
	: state_( state )
	, queue_( queue )
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
	queue_ptr get_queue( ) noexcept
	{
		return queue_;
	}

	/**
	 * The API:
	 *
	 * Below, E is either void, std::exception, or any other type for which
	 * exceptions will be tried to be caught.
	 * A generic_promise of type T... (actually tuple<T...>) has:
	 *
	 *   * then( T, ... )        -> X... => generic_promise< tuple< X > >
	 *   * then( tuple< T... > ) -> X... => generic_promise< tuple< X > >
	 *
	 *   * fail( E ) -> void // Can not be continued (except for done())
	 *   * fail( E ) -> generic_promise< tuple< T... > > // Can be continued, suitable for "retry" flow
	 *
	 *   * done( ) -> void
	 *
	 */

	/**
	 * ( ... ) -> value
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		this_type::template is_valid_arguments<
			Q_ARGUMENTS_OF( Fn )
		>::value
		&&
		!is_promise< Q_RESULT_OF( Fn ) >::value
		&&
		detail::temporary< queue_ptr >
			::template is_decayed< Queue >::value,
		promise< Q_RESULT_OF_AS_TUPLE_TYPE( Fn ) >
	>::type
	then( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * ( std::tuple< ... > ) -> value
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		::q::is_argument_same_or_convertible<
			arguments< tuple_type >, Q_ARGUMENTS_OF( Fn )
		>::value
		&&
		!is_promise< Q_RESULT_OF( Fn ) >::value
		&&
		detail::temporary< queue_ptr >
			::template is_decayed< Queue >::value,
		promise< Q_RESULT_OF_AS_ARGUMENT_TYPE( Fn )::tuple_type >
	>::type
	then( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * ( ... ) -> promise< value >
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		this_type::template is_valid_arguments<
			Q_ARGUMENTS_OF( Fn )
		>::value
		&&
		is_promise< Q_RESULT_OF( Fn ) >::value
		&&
		detail::temporary< queue_ptr >
			::template is_decayed< Queue >::value,
		Q_RESULT_OF( Fn )
	>::type
	then( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * ( std::tuple< ... > ) -> promise< value >
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		::q::is_argument_same_or_convertible<
			arguments< tuple_type >, Q_ARGUMENTS_OF( Fn )
		>::value
		&&
		is_promise< Q_RESULT_OF( Fn ) >::value
		&&
		detail::temporary< queue_ptr >
			::template is_decayed< Queue >::value,
		Q_RESULT_OF( Fn )
	>::type
	then( Fn&& fn, Queue&& queue = nullptr );

	template< typename Logger, typename Queue = queue_ptr >
	typename std::enable_if<
		is_same_type< Logger, log_chain_generator >::value
		&&
		detail::temporary< queue_ptr >
			::template is_decayed< Queue >::value,
		this_type
	>::type
	then( Logger&& logger, Queue&& queue = nullptr );

	/**
	 * Matches an exception as a raw std::exception_ptr
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		is_same_type<
			Q_FIRST_ARGUMENT_OF( Fn ),
			std::exception_ptr
		>::value &&
		std::is_void< Q_RESULT_OF( Fn ) >::value
		&&
		detail::temporary< queue_ptr >
			::template is_decayed< Queue >::value,
		unique_this_type
	>::type
	fail( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * Matches an exception as a raw std::exception_ptr
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		is_same_type<
			Q_FIRST_ARGUMENT_OF( Fn ),
			std::exception_ptr
		>::value &&
		is_promise< Q_RESULT_OF( Fn ) >::value
		&&
		detail::temporary< queue_ptr >
			::template is_decayed< Queue >::value,
		Q_RESULT_OF( Fn )
	>::type
	fail( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * Matches an exception of any type, defined by the one and only argument
	 * of fn
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		Q_ARITY_OF( Fn ) == 1 &&
		!Q_ARGUMENTS_ARE( Fn, std::exception_ptr )::value &&
		std::is_void< Q_RESULT_OF( Fn ) >::value
		&&
		detail::temporary< queue_ptr >
			::template is_decayed< Queue >::value,
		unique_this_type
	>::type
	fail( Fn&& fn, Queue&& queue = nullptr )
	{
		// TODO: Rewrite this and optimize for having multiple type matching
		// catches in a single try/catch block, so that the rethrowing only
		// happens once when performing many chained fail().fail().fail()
		// operations. Should be fairly simple, although allowing multiple
		// chained fail() could be tricky. An easier solution would be to make
		// this function take multiple functions, variadicly.

		std::exception_ptr eptr; // = get from somewhere
		typedef Q_FIRST_ARGUMENT_OF( Fn ) exception_type;

		auto tmp_fn = Q_TEMPORARILY_COPYABLE( fn );

		auto runner = [ tmp_fn, eptr ]( ) mutable
		{
			try
			{
				std::rethrow_exception( eptr );
			}
			catch ( const exception_type& e )
			{
				tmp_fn.consume( )( e );

				// Match, stop fail-chain as we've already caught the right
				// exception type.
				// If fn has thrown/rethrown, we'll not end up here, and
				// succeeding fail handlers will be called, as expected.
			}
		};

		state_.signal( ).push( std::move( runner ), queue );
	}

	/**
	 * A finally() function is always run, ignoring the current state, i.e.
	 * if the previous task threw an exception or returned a value.
	 *
	 * A normal use for finally() is to clean up.
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		std::is_void< Q_RESULT_OF( Fn ) >::value &&
		Q_ARITY_OF( Fn ) == 0
		&&
		detail::temporary< queue_ptr >
			::template is_decayed< Queue >::value,
		unique_this_type
	>::type
	finally( Fn&& fn, Queue&& queue = nullptr );

	void done( )
	{
		// TODO: Implement
	}

private:
	friend class ::q::promise< tuple_type >;
	friend class ::q::shared_promise< tuple_type >;

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

template< typename T >
class promise
: public detail::generic_promise< false, T >
{
	typedef promise< T >                        this_type;
	typedef detail::generic_promise< false, T > base_type;

public:
	typedef ::q::is_copy_constructible< T > shareable;

	promise( typename base_type::state_type&& state, const queue_ptr& queue )
	: base_type( std::move( state ), queue )
	{ }

	promise( ) = delete;
	promise( this_type&& ) = default;
	promise( const this_type& ) = delete;
	promise& operator=( this_type&& ) = default;
	promise& operator=( const this_type& ) = delete;

	promise( detail::generic_promise< false, T >&& ref )
	: base_type( std::move( ref.state_ ), ref.queue_ )
	{ }

	template< typename T_ = T >
	typename std::enable_if<
		shareable::value && std::is_same< T_, T >::value,
		shared_promise< T >
	>::type
	share( )
	{
		return shared_promise< T >(
			base_type::state_->acquire( ), this->get_queue( ) );
	}
};

template< typename T >
class shared_promise
: public detail::generic_promise< true, T >
{
	typedef shared_promise< T >                this_type;
	typedef detail::generic_promise< true, T > base_type;

public:
	shared_promise( typename base_type::state_type&& state,
	                const queue_ptr& queue )
	: base_type( std::move( state ), queue )
	{ }

	shared_promise(
		detail::promise_state_data< T, false >&& state,
		const queue_ptr& queue )
	: base_type( std::move( state ), queue )
	{ }

	shared_promise( detail::generic_promise< true, T >&& ref )
	: base_type( std::move( ref.state_ ), ref.queue_ )
	{ }

	shared_promise( ) = delete;
	shared_promise( this_type&& ) = default;
	shared_promise( const this_type& ) = default;
	shared_promise& operator=( this_type&& ) = default;
	shared_promise& operator=( const this_type& ) = default;

	promise< T > unshare( ) noexcept // TODO: analyze noexcept here
	{
		return promise< T >(
			base_type::state_->acquire( ), this->get_queue( ) );
	}
};

} // namespace q

#endif // LIBQ_PROMISE_PROMISE_HPP
