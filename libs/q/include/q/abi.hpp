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

#ifndef LIBQ_ABI_HPP
#define LIBQ_ABI_HPP

#include <string>
#include <typeinfo>

namespace q {

std::string demangle_cxx( const char* name );

template< typename T >
std::string demangle_type( )
{
	return demangle_cxx( typeid( T ).name( ) );
}

typedef std::string( *demangle_function )( const char* );

demangle_function register_demangle_cxx_function( demangle_function fn );

} // namespace q

#endif // LIBQ_ABI_HPP
