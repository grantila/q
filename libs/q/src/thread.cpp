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

#include "detail/cpu.hpp"

#ifdef LIBQ_ON_WINDOWS
#	include <windows.h>
	static thread_local std::string threadname_;
#elif defined( LIBQ_ON_OSX )
#elif defined( LIBQ_ON_POSIX )
#	include <pthread.h>
#endif

#if defined( LIBQ_ON_WINDOWS ) \
	|| defined( LIBQ_ON_POSIX ) \
	|| defined( LIBQ_ON_OSX )
#	define HAS_CUSTOM_CPU_INFO
#endif

#define AT_LEAST_ONE( n ) \
	std::max< std::size_t >( static_cast< std::size_t >( n ), 1 )

namespace q {

std::size_t fallback_cores( )
{
	return AT_LEAST_ONE( std::thread::hardware_concurrency( ) );
}

std::size_t hard_cores( )
{
#if defined( HAS_CUSTOM_CPU_INFO )
	return AT_LEAST_ONE( detail::get_cpu_info( ).hard_cores );
#else
	return fallback_cores( );
#endif
}

std::size_t soft_cores( )
{
#if defined( HAS_CUSTOM_CPU_INFO )
	return AT_LEAST_ONE( detail::get_cpu_info( ).soft_cores );
#else
	return fallback_cores( );
#endif
}

std::size_t processors( )
{
#if defined( HAS_CUSTOM_CPU_INFO )
	return detail::get_cpu_info( ).processors;
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
	if ( name.size( ) > 15 )
		pthread_setname_np( tid, name.substr( 0, 15 ).c_str( ) );
	else
		pthread_setname_np( tid, name.c_str( ) );
#endif

#elif defined( LIBQ_ON_WINDOWS )

	// Since Windows has no way of getting the thread name, once set, it
	// needs to be stored by q.
	threadname_ = name;

	const DWORD MS_VC_EXCEPTION = 0x406D1388;
#	pragma pack( push, 8 )
	typedef struct {
		DWORD dwType;
		LPCSTR szName;
		DWORD dwThreadID;
		DWORD dwFlags;
	} THREADNAME_INFO;
#	pragma pack( pop )

	THREADNAME_INFO info{ 0x1000, name.c_str( ), ~( DWORD )0, 0 };

#	pragma warning( push )
#	pragma warning( disable: 6320 6322 )
	__try
	{
		::RaiseException(
			MS_VC_EXCEPTION,
			0,
			sizeof( info ) / sizeof( ULONG_PTR ),
			reinterpret_cast< ULONG_PTR* >( &info ) );
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
	}
#	pragma warning( pop )

#endif
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

#elif defined( LIBQ_ON_WINDOWS )

	return threadname_;

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
