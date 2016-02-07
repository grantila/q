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
#elif defined( LIBQ_ON_OSX )
#	include <sys/types.h>
#	include <sys/sysctl.h>
#elif defined( LIBQ_ON_POSIX )
#	include <pthread.h>
#endif


#define AT_LEAST_ONE( n ) \
	std::max< std::size_t >( static_cast< std::size_t >( n ), 1 )

namespace q {

std::size_t fallback_cores( )
{
	return AT_LEAST_ONE( std::thread::hardware_concurrency( ) );
}

// TODO: Move and change implementation. This is a decent default, nothing else
std::size_t hard_cores( )
{
#if defined( LIBQ_ON_OSX )
	int count;
	size_t count_len = sizeof( count );
	::sysctlbyname( "hw.physicalcpu_max", &count, &count_len, NULL, 0 );
	return AT_LEAST_ONE( count );
#else
	return fallback_cores( );
#endif
}

std::size_t soft_cores( )
{
#if defined( LIBQ_ON_OSX )
	int count;
	size_t count_len = sizeof( count );
	::sysctlbyname( "hw.logicalcpu_max", &count, &count_len, NULL, 0 );
	return AT_LEAST_ONE( count );
#else
	return fallback_cores( );
#endif
}

std::size_t processors( )
{
#if defined( LIBQ_ON_OSX )
	int count;
	size_t count_len = sizeof( count );
	::sysctlbyname( "hw.packages", &count, &count_len, NULL, 0 );
	return static_cast< std::size_t >( count );
#else
	return 0;
#endif
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

std::string get_thread_name( )
{
#ifdef LIBQ_ON_POSIX

#	ifdef LIBQ_ON_LINUX
#		define PTHREAD_NAME_MAX 16
#	else
#		define PTHREAD_NAME_MAX NAME_MAX
#	endif

	auto tid = pthread_self( );
	char namebuf[ PTHREAD_NAME_MAX ];

#if defined( LIBQ_ON_OSX )
	pthread_getname_np( tid, namebuf, PTHREAD_NAME_MAX );
	return namebuf;
#elif defined( LIBQ_ON_BSD )
	pthread_get_name_np( tid, namebuf, PTHREAD_NAME_MAX );
	return namebuf;
#else
	pthread_getname_np( tid, namebuf, PTHREAD_NAME_MAX );
	return namebuf;
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
