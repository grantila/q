/*
 * Copyright 2016 Gustaf Räntilä
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

#include "cpu.hpp"

#ifdef LIBQ_ON_LINUX

#include "exec.hpp"
#include <stdlib.h>
#include <limits.h>
#include <algorithm>
#include <iostream>
#include <regex>
#include <thread>

namespace q {

namespace detail {

// C++ regex is pretty broken, since multiline support is implementation
// dependent, so we need to create our own '^' and '$' breaks...
#define RE_BOL "[\\n\\r^]"
#define RE_EOL "[\\n\\r$]"

cpu_info get_cpu_info( )
{
	cpu_info info{ 0, 0, 0, 0, 0, 0, 0 };

	std::string data = exec_read_all( "lscpu" );

	if ( data.empty( ) )
		return info;

	auto to_numeric = [ ]( const std::string& s ) -> std::size_t
	{
		std::size_t mul = 1;

		if ( s.empty( ) )
			return 0;

		if ( s[ s.size( ) - 1 ] == 'K' )
			mul = 1024;
		else if ( s[ s.size( ) - 1 ] == 'M' )
			mul = 1024 * 1024;

		auto val = ::strtoull( s.c_str( ), nullptr, 10 );
		if ( val == ULLONG_MAX )
			return 0;
		return static_cast< std::size_t >( val ) * mul;
	};

	auto get_value = [ & ]( const std::string& re ) -> std::size_t
	{
		auto re_cpu = std::regex( RE_BOL + re + RE_EOL );
		std::smatch match;
		std::regex_search( data, match, re_cpu );
		if ( match.size( ) == 2 )
			return to_numeric( match[ 1 ] );
		return 0;
	};

	auto soft_cores = get_value(
		"CPU\\(s\\):[[:space:]]*([[:digit:]]+)" );
	auto tpc = std::max( std::size_t( 1 ), get_value(
		"Threads\\(s\\) per core:[[:space:]]*([[:digit:]]+)" ) );
	auto l1c = get_value(
		"L1d cache:[[:space:]]*([[:digit:]]+)" );
	auto l2c = get_value(
		"L2 cache:[[:space:]]*([[:digit:]]+)" );
	auto l3c = get_value(
		"L3 cache:[[:space:]]*([[:digit:]]+)" );
	auto processors = get_value(
		"Socket\\(s\\):[[:space:]]*([[:digit:]]+)" );

	info.soft_cores = soft_cores;
	info.hard_cores = soft_cores / tpc;
	info.processors = processors;

	info.level_1_cache_size = l1c;
	info.level_2_cache_size = l2c;
	info.level_3_cache_size = l3c;

	return info;
}

} // namespace detail

} // namespace q

#endif // LIBQ_ON_LINUX
