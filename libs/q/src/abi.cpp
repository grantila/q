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

#include <atomic>
#include <cxxabi.h>

#include <cstdlib>

namespace q {

namespace {

std::atomic< demangle_function > _demangle_function( nullptr );

std::string default_demangle_cxx( const char* name )
{
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
