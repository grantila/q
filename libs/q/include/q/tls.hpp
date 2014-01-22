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

#ifndef LIBQ_TLS_HPP
#define LIBQ_TLS_HPP

#ifdef __clang__
#	define Q_NO_TLS_SUPPORT
#endif

#ifdef Q_NO_TLS_SUPPORT
#	include <q/mutex.hpp>
#	include <map>
#endif

namespace q {

template< typename T, typename Unique = T >
T& tls( )
{
#ifdef Q_NO_TLS_SUPPORT
#warning The emulated TLS support will leak memory as threads are created and destroyed.
	static mutex mut( Q_HERE, "Emulated TLS" );
	static std::map< std::thread::id, std::unique_ptr< T > > values;

	Q_AUTO_UNIQUE_LOCK( mut );
	auto iter = values.find( std::this_thread::get_id( ) );
	if ( iter == values.end( ) )
	{
		auto ut = std::unique_ptr< T >( new T );
		auto tid = std::this_thread::get_id( );
		auto tup = std::forward_as_tuple(
			std::move( tid ), std::move( ut ) );
		iter = values.insert( std::move( tup ) ).first;
	}
	T& t = *values[ std::this_thread::get_id( ) ];
#else
	thread_local T t;
#endif
	return t;
}

} // namespace q

#endif // LIBQ_TLS_HPP
