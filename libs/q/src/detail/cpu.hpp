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
#include <cstdint>

namespace q {

namespace detail {

struct cpu_info
{
	std::size_t processors;
	std::size_t hard_cores;
	std::size_t soft_cores;
	std::uint64_t level_1_cache_size;
	std::uint64_t level_2_cache_size;
	std::uint64_t level_3_cache_size;
	std::size_t cache_line_size;
};

cpu_info get_cpu_info( );

} // namespace detail

} // namespace q

#endif // LIBQ_INTERNAL_CPU_HPP
