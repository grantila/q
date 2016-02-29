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

#ifdef LIBQ_ON_WINDOWS

#include <iostream>

#include <windows.h>
#define SLPI_SIZE sizeof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION )

namespace q {

namespace detail {

cpu_info get_cpu_info( )
{
	cpu_info info{ 0, 0, 0, 0, 0, 0, 0 };

	DWORD bufsize = 0;

	::GetLogicalProcessorInformation( nullptr, &bufsize );

	std::size_t count = bufsize / SLPI_SIZE;

	if ( count * SLPI_SIZE != bufsize )
		return info;

	std::vector< SYSTEM_LOGICAL_PROCESSOR_INFORMATION > slpi( count );
	auto ok = ::GetLogicalProcessorInformation( &slpi[ 0 ], &bufsize );
	if ( !ok )
		return info;

	std::vector< std::size_t > relations( 64, 0 );

	auto mask_to_indexes = [ ]( std::uint64_t mask ) -> std::vector< std::size_t >
	{
		std::vector< std::size_t > indexes;

		for ( std::size_t i = 0; i < 64; ++i )
			if ( mask & ( std::uint64_t( 1 ) << i ) )
				indexes.push_back( i );

		return indexes;
	};

	// There is no OS agnostic common API for this, so this loop is not
	// optimally detecting the architecture, but it gets the number of
	// cores and cache details for at least some of them.
	for ( auto& part : slpi )
	{
		if ( part.Relationship == RelationProcessorCore )
		{
			if ( part.ProcessorCore.Flags == 1 )
				// The all share the same group (SMT)
				++info.hard_cores;

			auto indexes = mask_to_indexes(
				static_cast< std::uint64_t >(
					part.ProcessorMask ) );

			for ( auto index : indexes )
			{
				++info.soft_cores;
				if ( part.ProcessorCore.Flags != 1 )
					++info.hard_cores;
				relations[ index ] = info.hard_cores;
			}
		}
		if ( part.Relationship == RelationCache )
		{
			if ( part.Cache.Level == 1 )
				info.level_1_cache_size =
					static_cast< std::uint64_t >(
						part.Cache.Size );
			else if ( part.Cache.Level == 2 )
				info.level_2_cache_size =
					static_cast< std::uint64_t >(
						part.Cache.Size );
			else if ( part.Cache.Level == 3 )
				info.level_2_cache_size =
					static_cast< std::uint64_t >(
						part.Cache.Size );
			else
				continue;

			if ( info.cache_line_size == 0 )
				info.cache_line_size =
					static_cast< std::size_t >(
						part.Cache.LineSize );
		}
		if ( part.Relationship == RelationProcessorPackage )
		{
			// Each time this value is presented, we get a new
			// "processor package" (processor in english). So this
			// should count up to the number of processors.
			++info.processors;
		}
	}

	return info;
}

} // namespace detail

} // namespace q

#endif // LIBQ_ON_WINDOWS
