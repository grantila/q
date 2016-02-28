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

#include <q/abi.hpp>
#include <q/pp.hpp>

#include <atomic>
#include <cstdlib>

#ifdef LIBQ_ON_WINDOWS
#	include <Windows.h>
#	pragma warning( push )
#	pragma warning( disable: 4091 )
#	include "DbgHelp.h"
#	pragma warning( pop )
#	include <q/mutex.hpp>
#else
#	include <cxxabi.h>
#endif

namespace q {

namespace {

std::atomic< demangle_function > _demangle_function( nullptr );

#ifdef LIBQ_ON_WINDOWS
	q::mutex win32_mutex;
	const int win32_buflen = 4096;
	char win32_buf[ win32_buflen ];
#endif

std::string default_demangle_cxx( const char* name )
{
	if ( name[ 0 ] == 0 )
		return name;

#ifdef LIBQ_ON_WINDOWS
	Q_AUTO_UNIQUE_LOCK( win32_mutex );
	auto len = UnDecorateSymbolName(
		name, win32_buf, win32_buflen, UNDNAME_COMPLETE );
	if ( len > 0 )
		return std::string( win32_buf, len );
	else
		// Some error occured - ignore and use mangled name
		return name;
#else
	int status;
	char* demangled_name;

	demangled_name = abi::__cxa_demangle( name, 0, 0, &status );

	if ( status == 0)
	{
		std::string ret( demangled_name );
		std::free( demangled_name );

		return std::move( ret );
	}

	// Something went wrong with the demangling (memory issue or bad name),
	// so just return the raw string, mangled.
	return name;
#endif
}

} // anonymous namespace

std::string demangle_cxx( const char* name )
{
	demangle_function fn = _demangle_function.load(
		std::memory_order_seq_cst );

	if ( fn )
		return fn( name );

	return default_demangle_cxx( name );
}

demangle_function register_demangle_cxx_function( demangle_function fn )
{
	return _demangle_function.exchange( fn, std::memory_order_seq_cst );
}

} // namespace q
