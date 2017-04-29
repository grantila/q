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

namespace {

static inline std::size_t fallback_msb( std::uint64_t num )
{
	for ( int i = 63; i >= 0; --i )
		if ( num & ( std::uint64_t( 1 ) << i ) )
			return static_cast< std::size_t >( i );
	return 0;
}

} // empty namespace

#ifdef LIBQ_ON_WINDOWS
template< typename type, std::size_t size = sizeof( type ) >
typename std::enable_if< ( size < 5 ), std::size_t >::type
msb_win32( type num )
{
	unsigned long ret;
	_BitScanReverse( &ret, static_cast< unsigned long >( num ) );
	return ret;
}
template< typename type, std::size_t size = sizeof( type ) >
typename std::enable_if< ( size > 4 ), std::size_t >::type
msb_win32( type num )
{
#	ifdef LIBQ_ON_X64
	unsigned long ret;
	_BitScanReverse64( &ret, static_cast< unsigned __int64 >( num ) );
	return ret;
#	else
	return fallback_msb( num );
#	endif
}
#endif // LIBQ_ON_WINDOWS

template< typename type, std::size_t size = sizeof( type ) >
typename std::enable_if<
#ifdef LIBQ_ON_GCC_OR_CLANG
	sizeof( type ) <= sizeof( long long int ),
#else
	true,
#endif
	std::size_t
>::type
msb( type&& _num )
{
#ifndef LIBQ_ON_WINDOWS
	auto num = static_cast<
#ifdef LIBQ_ON_GCC_OR_CLANG
		unsigned long long int
#else
		std::uint64_t
#endif
	>( _num );
#endif

#ifdef LIBQ_ON_GCC_OR_CLANG
	return static_cast< std::size_t >(
		( 8 * sizeof( unsigned long long int ) ) -
		__builtin_clzll( num ) );
#elif defined( LIBQ_ON_WINDOWS )
	return msb_win32( _num );
#else // Fallback solution
	return fallback_msb( _num );
#endif
}

#if !defined(LIBQ_ON_ANDROID)

template< typename type, std::size_t size = sizeof( type ) >
typename std::enable_if< size <= sizeof( long long int ), std::size_t >::type
lsb( type&& _num )
{
	auto num = static_cast< unsigned long long int >( _num );

	return static_cast< std::size_t >( ffsll( num ) );
}

#endif

} // namespace detail

} // namespace q

#endif // LIBQ_INTERNAL_NUMERIC_HPP
