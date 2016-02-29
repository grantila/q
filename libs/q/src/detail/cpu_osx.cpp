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

#ifdef LIBQ_ON_OSX

#include <sys/types.h>
#include <sys/sysctl.h>

namespace q {

namespace detail {

cpu_info get_cpu_info( )
{
	cpu_info info{ 0, 0, 0, 0, 0, 0, 0 };

	std::int32_t count32;
	std::int64_t count64;
	size_t count32_len = sizeof( count32 );
	size_t count64_len = sizeof( count64 );

	::sysctlbyname( "hw.physicalcpu_max", &count32, &count32_len, NULL, 0 );
	info.hard_cores = static_cast< std::size_t >( count32 );

	::sysctlbyname( "hw.logicalcpu_max", &count32, &count32_len, NULL, 0 );
	info.soft_cores = static_cast< std::size_t >( count32 );

	::sysctlbyname( "hw.packages", &count32, &count32_len, NULL, 0 );
	info.processors = static_cast< std::size_t >( count32 );


	::sysctlbyname( "l1dcachesize", &count64, &count64_len, NULL, 0 );
	info.level_1_cache_size = static_cast< std::uint64_t >( count64 );

	::sysctlbyname( "l2cachesize", &count64, &count64_len, NULL, 0 );
	info.level_2_cache_size = static_cast< std::uint64_t >( count64 );

	::sysctlbyname( "l3cachesize", &count64, &count64_len, NULL, 0 );
	info.level_3_cache_size = static_cast< std::uint64_t >( count64 );

	::sysctlbyname( "hw.cachelinesize", &count64, &count64_len, NULL, 0 );
	info.cache_line_size = static_cast< std::size_t >( count64 );

	return info;
}

} // namespace detail

} // namespace q

#endif // LIBQ_ON_OSX
