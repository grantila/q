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

#ifndef LIBQ_RX_OBSERVABLE_OBSERVABLE_IMPL_FROM_HPP
#define LIBQ_RX_OBSERVABLE_OBSERVABLE_IMPL_FROM_HPP

#include <q/channel.hpp>

namespace q { namespace rx {

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

template< typename T >
template< typename Queue, typename Tag >
observable< T >
observable< T >::
from(
	std::iterator< Tag, T > begin,
	std::iterator< Tag, T > end,
	Queue&& queue
)
{
	;
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_OBSERVABLE_IMPL_FROM_HPP
