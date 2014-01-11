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

#include <q/thread.hpp>
#include <q/pp.hpp>

#ifdef LIBQ_ON_WINDOWS
  // todo
#elif defined( LIBQ_ON_POSIX )
#	include <pthread.h>
#endif


namespace q {

// TODO: Move and change implementation. This is a decent default, nothing else
std::size_t hard_cores( )
{
	return static_cast< std::size_t >( std::thread::hardware_concurrency( ) );
}

std::size_t soft_cores( )
{
	return hard_cores( );
}

namespace detail {

void set_thread_name( const std::string& name )
{
#ifdef LIBQ_ON_POSIX

#ifndef LIBQ_ON_OSX
	auto tid = pthread_self( );
#endif // LIBQ_ON_OSX

#if defined( LIBQ_ON_OSX )
	pthread_setname_np( name.c_str( ) );
#elif defined( LIBQ_ON_BSD )
	pthread_set_name_np( tid, name.c_str( ) );
#else
	pthread_setname_np( tid, name.c_str( ) );
#endif

#endif // LIBQ_ON_POSIX
}

} // namespace detail

/*
thread* thread::current( )
{
	// TODO: TLS Lookup, or nullptr
	return nullptr;
}
*/

} // namespace q
