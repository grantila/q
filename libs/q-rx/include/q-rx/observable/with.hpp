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

#ifndef LIBQ_RX_OBSERVABLE_WITH_HPP
#define LIBQ_RX_OBSERVABLE_WITH_HPP

#include <q/channel.hpp>

namespace q { namespace rx {

/**
 * Main entry point for starting an observable stream based on an existing
 * container, an iterator pair or a channel.
 */

template< typename T >
observable< T >
with( ::q::channel< T > ch, queue_ptr queue = nullptr )
{
	return observable< T >::from( ch, std::move( queue ) );
}

template< typename T, typename Queue, typename... Any >
observable< T >
with( std::vector< T, Any... > container, Queue&& queue )
{
	return observable< T >::from(
		std::move( container ),
		std::forward< Queue >( queue )
	);
}

template< typename T, typename Queue, typename... Any >
observable< T >
with( std::list< T, Any... > container, Queue&& queue )
{
	return observable< T >::from(
		std::move( container ),
		std::forward< Queue >( queue )
	);
}

template< typename T, typename Queue, typename Tag >
observable< T >
with(
	std::iterator< Tag, T > begin,
	std::iterator< Tag, T > end,
	Queue&& queue
)
{
	return observable< T >::from(
		std::move( begin ),
		std::move( end ),
		std::forward< Queue >( queue )
	);
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_WITH_HPP
