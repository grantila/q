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

#ifndef LIBQ_INTERNAL_CPU_HPP
#define LIBQ_INTERNAL_CPU_HPP

#include <q/pp.hpp>
#include "cpu.hpp"

#include <vector>

namespace q {

namespace detail {

enum class cache_level
{
	level_1,
	level_2,
	level_3
};

struct cpu_cache {
	std::size_t size;
	std::size_t line_size;
};

struct cpu_info
{
	std::size_t processors;
	std::size_t hard_cores;
	std::size_t soft_cores;
	cpu_cache level_1_cache;
	cpu_cache level_2_cache;
	cpu_cache level_3_cache;

	std::size_t get_cache_size( cache_level level ) const
	{
		if ( level == cache_level::level_1 )
			return level_1_cache.size;
		else if ( level == cache_level::level_2 )
			return level_2_cache.size;
		else if ( level == cache_level::level_3 )
			return level_3_cache.size;
		else
			return 0;
	}

	std::size_t get_cache_line_size( cache_level level ) const
	{
		if ( level == cache_level::level_1 )
			return level_1_cache.line_size;
		else if ( level == cache_level::level_2 )
			return level_2_cache.line_size;
		else if ( level == cache_level::level_3 )
			return level_3_cache.line_size;
		else
			return 0;
	}
};

cpu_info get_cpu_info( );

} // namespace detail

} // namespace q

#endif // LIBQ_INTERNAL_CPU_HPP
