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

template< typename T >
class observable
{
public:
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

	observable( q::readable< T >&& readable )
	: readable_( std::make_shared<
		detail::observable_readable_direct< T >
	>( std::move( readable ) ) )
	{ }

	observable( q::channel< T > channel )
	: observable( channel.get_readable( ) )
	{ }

	observable( q::readable< q::expect< T > >&& readable )
	: readable_( std::make_shared<
		detail::observable_readable_expect< T >
	>( std::move( readable ) ) )
	{ }

	observable( q::channel< q::expect< T > > channel )
	: observable( channel.get_readable( ) )
	{ }

	observable( q::readable< q::promise< std::tuple< T > > >&& readable )
	: readable_( std::make_shared<
		detail::observable_readable_promise< T >
	>( std::move( readable ) ) )
	{ }

	observable( q::channel< q::promise< std::tuple< T > > > channel )
	: observable( channel.get_readable( ) )
	{ }

	observable( q::readable< q::shared_promise< std::tuple< T > > >&& readable )
	: readable_( std::make_shared<
		detail::observable_readable_shared_promise< T >
	>( std::move( readable ) ) )
	{ }

	observable( q::channel< q::shared_promise< std::tuple< T > > > channel )
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
	 * Creators: From
	 ***********************************************/

	static observable< T > from( q::channel< T > channel );

	template< typename U, typename Queue >
	static typename std::enable_if<
		std::is_same< std::vector< T >, typename std::decay< U >::type >::value // TODO: Change into q::is_container of some kind
		,
		observable< T >
	>::type
	from( U&& container, Queue&& queue = nullptr );

	template< typename Queue, typename Tag >
	static observable< T > from(
		std::iterator< Tag, T > begin,
		std::iterator< Tag, T > end,
		Queue&& queue
	);


	/**********************************************************************
	 * Transformers
	 *********************************************************************/

	/************************************************
	 * Transformers: Map
	 ***********************************************/

	/**
	 * ( In ) -> Out
	 */
	template< typename Fn >
	typename std::enable_if<
		!q::is_promise< Q_RESULT_OF( Fn ) >::value,
		observable< Q_RESULT_OF( Fn ) >
	>::type
	map( Fn&& fn, base_options options = base_options( ) );

	/**
	 * ( In ) -> promise< Out >
	 */
	template< typename Fn >
	typename std::enable_if<
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
	 * ( T ) -> void
	 */
	template< typename Fn >
	typename std::enable_if<
		Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( Fn, T )::value
		and
		std::is_void< ::q::result_of< Fn > >::value,
		::q::promise< std::tuple< > >
	>::type
	consume( Fn&& fn, base_options options = base_options( ) );

	/**
	 * ( T ) -> promise< >
	 *
	 * This will make q-rx await the returned promise, and continue only
	 * when the promise is resolved. It causes proper back pressure
	 * handling and will also stop if the promise is rejected.
	 */
	template< typename Fn >
	typename std::enable_if<
		Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( Fn, T )::value
		and
		::q::is_promise<
			typename std::decay< ::q::result_of< Fn > >::type
		>::value
		and
		::q::result_of< Fn >::argument_types::size::value == 0,
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

//	q::readable< T > readable_;
	std::shared_ptr< detail::observable_readable< T > > readable_;
};

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_OBSERVABLE_HPP
