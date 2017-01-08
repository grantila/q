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

#ifndef LIBQ_INTERNAL_STACKTRACE_HPP
#define LIBQ_INTERNAL_STACKTRACE_HPP

#include <q/stacktrace.hpp>

#include <algorithm>
#include <cstring>
#include <cstdlib>

namespace q {

namespace detail {

static inline std::ptrdiff_t hex_to_dec( const char* s )
{
        auto ret = ::strtoull( s, nullptr, 16 );
        return static_cast< std::ptrdiff_t >( ret );
}

// Extracts <filename, path> from the full filename
static inline std::pair< std::string, std::string >
get_file_name( const char* file_name )
{
        auto last_back_slash = std::strrchr( file_name, '\\' );
        auto last_slash = std::strrchr( file_name, '/' );
        auto best_found = std::max( last_back_slash, last_slash );
        auto name = std::max(
		file_name,
		static_cast< const char* >( best_found + 1 ) );

	return std::make_pair(
		std::string( name ),
		std::string( file_name, name - file_name )
	);
}

::q::stacktrace::frame parse_stack_frame( const char* data )
noexcept;

stacktrace default_stacktrace( ) noexcept;

} // namespace detail

} // namespace q

#endif // LIBQ_INTERNAL_STACKTRACE_HPP
