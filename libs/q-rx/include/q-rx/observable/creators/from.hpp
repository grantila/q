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

#ifndef LIBQ_RX_OBSERVABLE_CREATORS_FROM_HPP
#define LIBQ_RX_OBSERVABLE_CREATORS_FROM_HPP

namespace q { namespace rx {

template< typename T >
inline observable< T > observable< T >::
from( q::channel< T > channel )
{
	return observable< T >( channel );
}

template< typename T >
template< typename U, typename Queue >
inline typename std::enable_if<
	q::is_container_v< std::decay_t< U > >
	and
	std::is_same<
		objectify_t< typename std::decay_t< U >::value_type >,
		objectify_t< T >
	>::value,
	observable< T >
>::type
observable< T >::
from( U&& container, Queue&& queue )
{
	auto channel_ = channel_type( queue, container.size( ) );
	auto writable = channel_.get_writable( );
	for ( auto& val : container )
		writable.send( val );
	return observable< T >( channel_ );
}

template< typename T >
template< typename Queue, typename Tag, typename U >
typename std::enable_if<
	!std::is_void< U >::value,
	observable< T >
>::type
observable< T >::
from(
	std::iterator< Tag, U > begin,
	std::iterator< Tag, U > end,
	Queue&& queue
)
{
	// TODO: Change to typename Iterator, and checking Iterator::value_type
	throw "not implemented yet";
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_CREATORS_FROM_HPP
