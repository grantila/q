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

#ifndef LIBQ_INTERNAL_NUMERIC_HPP
#define LIBQ_INTERNAL_NUMERIC_HPP

#include <q/pp.hpp>

#ifdef LIBQ_ON_WINDOWS
#	include <intrin.h>
#else
#	include <string.h>
#endif

namespace q {

namespace detail {

template<
	typename integer_type,
	std::size_t integer_size = sizeof( integer_type )
>
typename std::enable_if<
#ifdef LIBQ_ON_GCC
	sizeof( integer_type ) <= sizeof( long long int ),
#else
	true,
#endif
	std::size_t
>::type
msb( integer_type&& _num )
{
	auto num = static_cast<
#ifdef LIBQ_ON_GCC
		unsigned long long int
#elif defined( LIBQ_ON_WINDOWS )
		typename std::conditional<
			( sizeof( integer_type ) < 5 ),
			unsigned long,
			unsigned __int64
		>::type
#else
		std::uint64_t
#endif
	>( _num );

#ifdef LIBQ_ON_GCC
	return static_cast< std::size_t >(
		( 8 * sizeof( unsigned long long int ) ) -
		__builtin_clzll( num ) );
#elif defined( LIBQ_ON_WINDOWS )
	unsigned long ret;
	if ( sizeof( integer_type ) < 5 )
		_BitScanReverse( &ret, num );
	else
		_BitScanReverse64( &ret, num );
	return ret;
#else
	for ( int i = 63; i >= 0; --i )
		if ( num & ( std::uint64_t( 1 ) << i ) )
			return static_cast< std::size_t >( i );
#endif
}

template<
	typename integer_type,
	std::size_t integer_size = sizeof( integer_type )
>
typename std::enable_if<
	sizeof( integer_type ) <= sizeof( long long int ),
	std::size_t
>::type
lsb( integer_type&& _num )
{
	auto num = static_cast< unsigned long long int >( _num );

	return static_cast< std::size_t >( ffsll( num ) );
}

} // namespace detail

} // namespace q

#endif // LIBQ_INTERNAL_NUMERIC_HPP
