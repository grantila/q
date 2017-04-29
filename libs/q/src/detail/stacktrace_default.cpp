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

#include <q/pp.hpp>
#include <q/abi.hpp>

#include "stacktrace.hpp"

#if !defined(LIBQ_ON_WINDOWS) && !defined(LIBQ_ON_ANDROID)
#	include <execinfo.h>
#endif

namespace q {

namespace detail {

#if !defined( LIBQ_ON_WINDOWS ) && \
	!defined( LIBQ_ON_LINUX ) && \
	!defined( LIBQ_ON_OSX )

stacktrace::frame parse_stack_frame( const char* data )
noexcept
{
	return stacktrace::frame{ 0, "", 0, 0, data, "" };
}

#endif

#if !defined(LIBQ_ON_WINDOWS) && !defined(LIBQ_ON_ANDROID)

stacktrace default_stacktrace( ) noexcept
{

	static const std::size_t buflen = 128;
	void* addresses[ buflen ];
	std::size_t size = backtrace( addresses, buflen );

	char** raw_frames = backtrace_symbols( addresses, size );

	std::vector< stacktrace::frame > frames;
	frames.reserve( size );

	for ( std::size_t i = 0; i < size; ++i )
	{
		auto frame = parse_stack_frame( raw_frames[ i ] );
		frame.frameno = i;
		frame.symbol = demangle_cxx( frame.symbol.c_str( ) );
		frames.push_back( frame );
	}

	::free( raw_frames );

	return stacktrace( std::move( frames ) );
}

#elif defined(LIBQ_ON_ANDROID)
stacktrace default_stacktrace( ) noexcept
{
    return stacktrace(std::vector< stacktrace::frame >());
}

#endif

} // namespace detail

} // namespace q
