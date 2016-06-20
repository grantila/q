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

#include <q/channel.hpp>

namespace q { namespace rx {

template< typename T >
class observable
{
public:
	observable( )
	{ }

	observable( q::readable< T >&& readable )
	: readable_( std::move( readable ) )
	{ }

	observable( q::channel< T > channel )
	: observable( channel.get_readable( ) )
	{ }

	// Construction methods

	static observable< T > empty( const q::queue_ptr& queue );

	static observable< T > never( const q::queue_ptr& queue );

	static observable< T > from( q::channel< T > channel );

	template< typename U, typename Queue >
	static typename std::enable_if<
		std::is_same< std::vector< T >, typename std::decay< U >::type >::value // TODO: Change into q::is_container of some kind
		,
		observable< T >
	>::type
	from( U&& container, Queue&& queue = nullptr );


	// Transformation operators

	/**
	 * ( In ) -> promise< Out >
	 */
	template< typename Fn >
	typename std::enable_if<
		q::is_promise< Q_RESULT_OF( Fn ) >::value,
		observable<
			typename std::conditional<
				std::tuple_size<
					typename ::q::result_of< Fn >::tuple_type
				>::value == 0,
				void,
				typename std::conditional<
					std::tuple_size<
						typename ::q::result_of< Fn >::tuple_type
					>::value == 1,
					typename std::tuple_element<
						0,
						typename ::q::result_of< Fn >::tuple_type
					>::type,
					typename ::q::result_of< Fn >::tuple_type
				>::type
			>::type
		>
	>::type
	map( Fn&& fn );

	// Consuming (instead of onNext, onError, onComplete)

	/**
	 * ( T ) -> void
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( Fn, T )::value
		and
		std::is_void< ::q::result_of< Fn > >::value
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		::q::promise< std::tuple< > >
	>::type
	consume( Fn&& fn, Queue&& queue = nullptr );

	/**
	 * ( T ) -> promise< >
	 *
	 * This will make q-rx await the returned promise, and continue only
	 * when the promise is resolved. It causes proper back pressure
	 * handling and will also stop if the promise is rejected.
	 */
	template< typename Fn, typename Queue = queue_ptr >
	typename std::enable_if<
		/*Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( Fn, T )::value
		and*/
		::q::is_promise< typename std::decay< ::q::result_of< Fn > >::type >::value
		and
		::q::result_of< Fn >::argument_types::size::value == 0
		and
		Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
		::q::promise< std::tuple< > >
	>::type
	consume( Fn&& fn, Queue&& queue = nullptr );

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
		return readable_.get_queue( );
	}

	q::readable< T > readable_;
};

template< typename T >
inline observable< T > observable< T >::empty( const q::queue_ptr& queue )
{
	q::channel< T > channel_( queue, 0 );
	channel_.get_writable( ).close( );
	return observable< T >( channel_ );
}

template< typename T >
inline observable< T > observable< T >::never( const q::queue_ptr& queue )
{
	auto channel_ = q::channel< T >( queue, 1 );
	return observable< T >( channel_ );
}

template< typename T >
inline observable< T > observable< T >::from( q::channel< T > channel )
{
	return observable< T >( channel );
}

template< typename T >
template< typename U, typename Queue >
inline typename std::enable_if<
	std::is_same< std::vector< T >, typename std::decay< U >::type >::value
	,
	observable< T >
>::type
observable< T >::
from( U&& container, Queue&& queue )
{
	auto channel_ = q::channel< T >( queue, container.size( ) );
	auto writable = channel_.get_writable( );
	for ( auto& val : container )
		writable.send( val );
	return observable< T >( channel_ );
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_OBSERVABLE_HPP
