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

#include "detail/stacktrace.hpp"
#include <q/abi.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cstring>

#ifndef LIBQ_ON_WINDOWS
#	include <execinfo.h>
#endif

namespace q {

stacktrace::stacktrace( std::vector< frame >&& frames )
noexcept
: frames_( std::move( frames ) )
{ }

const std::vector< stacktrace::frame >& stacktrace::frames( ) const
noexcept
{
	return frames_;
}

std::string stacktrace::string( ) const
noexcept
{
	try
	{
		std::stringstream ss;
		ss << *this;
		return std::move( ss.str( ) );
	}
	catch ( ... )
	{
		return std::string( );
	}
}

std::ostream& operator<<( std::ostream& os, const stacktrace& st )
noexcept
{
	std::size_t max_frame = 0;
	std::size_t max_lib   = 0;
	std::size_t max_addr  = 0;

	for ( auto& frame : st.frames( ) )
	{
		if ( max_frame < frame.frame )
			max_frame = frame.frame;
		if ( max_lib < frame.lib.size( ) )
			max_lib = frame.lib.size( );
		if ( max_addr < frame.addr.size( ) )
			max_addr = frame.addr.size( );
	}

	char buf[16];
	snprintf( buf, sizeof buf, "%lu", max_frame );
	max_frame = std::strlen( buf );

	max_lib += 1;

	for ( auto& frame : st.frames( ) )
		os << "Frame " << std::setfill( ' ' )
		<< std::setw( max_frame ) << frame.frame << ": "
		<< std::setw( max_lib ) << std::left << frame.lib << std::right
		<< std::setw( max_addr ) << frame.addr << " "
		<< frame.symbol << " "
		<< frame.extra << std::endl;
	return os;
}

namespace {

std::atomic< stacktrace_function > _stacktrace_function( nullptr );

stacktrace default_stacktrace( )
{
	static const std::size_t buflen = 128;
	void* addresses[ buflen ];
	std::size_t size = backtrace( addresses, buflen );

	char** raw_frames = backtrace_symbols( addresses, size );

	std::vector< stacktrace::frame > frames;
	frames.reserve( size );

	for ( std::size_t i = 0; i < size; ++i )
	{
		auto frame = ::q::detail::parse_stack_frame( raw_frames[ i ] );
		frame.frame = i;
		frame.symbol = demangle_cxx( frame.symbol.c_str( ) );
		frames.push_back( frame );
	}

	return stacktrace( std::move( frames ) );
}

} // anonymous namespace

stacktrace_function register_stacktrace_function( stacktrace_function fn )
noexcept
{
	return _stacktrace_function.exchange( fn, std::memory_order_seq_cst );
}

stacktrace get_stacktrace( )
noexcept
{
	stacktrace_function fn = _stacktrace_function.load(
		std::memory_order_seq_cst );

	try
	{
		if ( fn )
			return fn( );

		return default_stacktrace( );
	}
	catch ( ... )
	{
		std::vector< stacktrace::frame > frames;
		return stacktrace( std::move( frames ) );
	}
}

} // namespace q
