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

	observable( q::readable_ptr< T >&& readable )
	: readable_( std::move( readable ) )
	{ }

	// Construction methods

	static observable< T > empty( const q::queue_ptr& queue );

	static observable< T > never( const q::queue_ptr& queue );

	static observable< T > from( q::channel_ptr< T > channel );

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
	template< typename Fn >
	typename std::enable_if<
		std::is_void< ::q::result_of< Fn > >::value,
		::q::promise< std::tuple< > >
	>::type
	consume( Fn&& fn );

	/**
	 * ( T ) -> promise< >
	 *
	 * This will make q-rx await the returned promise, and continue only
	 * when the promise is resolved. It causes proper back pressure
	 * handling and will also stop if the promise is rejected.
	 */
	template< typename Fn >
	typename std::enable_if<
		::q::is_promise< ::q::result_of< Fn > >::value
		and
		::q::result_of< Fn >::argument_types::size == 0,
		::q::promise< std::tuple< > >
	>::type
	consume( Fn&& fn );

	// TODO: Implemement properly, e.g. with promises
	T onNext( );
	std::exception_ptr onError( );
	void onComplete( );

private:
	q::readable_ptr< T > readable_;
};

// TODO: Move to channel implementation
template< typename T, typename Queue >
q::channel_ptr< T > make_channel( Queue&& queue, std::size_t buffer_count )
{
	return std::make_shared< q::channel< T > >( std::forward< Queue >( queue ), buffer_count );
}

template< typename T >
inline observable< T > observable< T >::empty( const q::queue_ptr& queue )
{
	auto channel_ = make_channel< T >( queue, 0 );
	channel_->close( );
	return observable< T >( channel_ );
}

template< typename T >
inline observable< T > observable< T >::never( const q::queue_ptr& queue )
{
	auto channel_ = make_channel< T >( queue, 1 );
	return observable< T >( channel_ );
}

template< typename T >
inline observable< T > observable< T >::from( q::channel_ptr< T > channel )
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
	auto channel_ = make_channel< T >( queue, container.size( ) );
	for ( auto& val : container )
		channel_->send( val );
	return observable< T >( channel_ );
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_OBSERVABLE_HPP
