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

#ifndef LIBQ_STATIC_ATOMIC_HPP
#define LIBQ_STATIC_ATOMIC_HPP

#include <q/mutex.hpp>

#include <atomic>
#include <memory>

namespace q {

/**
 * static_atomic< T > is a helper function primarily targeting atomic access to
 * staticly initialized data. For any type T, static_atomic( ) can ensure one
 * instance of the type being initialized and is quickly retrievable without
 * any mutex locks. Mutexes are only used to ensure atomic instanciation, and
 * even this is subject to change into being non-locking.
 */
template< typename T > std::shared_ptr< T > static_atomic( )
{
	static mutex state_mutex_;
	static std::shared_ptr< T > state_;

	auto s = std::atomic_load( &state_ );

	if ( !s )
	{
		Q_AUTO_UNIQUE_LOCK( state_mutex_ );

		auto s2 = std::atomic_load( &state_ );
		if ( !s2 )
		{
			s2 = std::make_shared< T >( );
			std::atomic_store( &state_, s2 );
		}

		return s2;
	}

	return s;
}

} // namespace q

#endif // LIBQ_STATIC_ATOMIC_HPP
