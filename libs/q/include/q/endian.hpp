/*
 * Copyright 2014 Gustaf Räntilä
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

#ifndef LIBQ_ENDIAN_HPP
#define LIBQ_ENDIAN_HPP

#include <q/pp.hpp>

#include <cstddef>
#include <cstdint>

#ifdef LIBQ_ON_LINUX
#	include <byteswap.h>
#	define LIBQ_SWAP_16 bswap_16
#	define LIBQ_SWAP_32 bswap_32
#	define LIBQ_SWAP_64 bswap_64
#else
#	define LIBQ_SWAP_CHUNK( chunk, x ) \
		( ( ( x ) >> chunk ) | ( ( x ) << chunk ) )
#	define LIBQ_SWAP_16( x ) \
		LIBQ_SWAP_CHUNK( 8, x )
#	define LIBQ_SWAP_32( x ) \
		LIBQ_SWAP_CHUNK( 16, \
			( ( ( x ) << 8 ) & 0xFF00FF00 ) | \
			( ( ( x ) >> 8 ) & 0xFF00FF ) \
		)
#	define LIBQ_SWAP_64_HALF( x ) \
			( ( ( x ) << 16 ) & 0xFFFF0000FFFF0000ULL ) | \
			( ( ( x ) >> 16 ) & 0x0000FFFF0000FFFFULL )
#	define LIBQ_SWAP_64( x ) \
		LIBQ_SWAP_CHUNK( 32, \
			LIBQ_SWAP_64_HALF( \
				( ( ( x ) << 8 ) & 0xFF00FF00FF00FF00ULL ) | \
				( ( ( x ) >> 8 ) & 0x00FF00FF00FF00FFULL ) \
			) \
		)
#endif

#define LIBQ_SWAP_ANY( x, size ) \
	( size == 1 ) \
	? ( x ) \
	: ( size == 2 ) \
	  ? LIBQ_SWAP_16( x ) \
	  : ( size == 4 ) \
	    ? LIBQ_SWAP_32( x ) \
	    : ( size == 8 ) \
	      ? LIBQ_SWAP_64( x ) \
	      : ( x ) // Error

namespace q {

namespace detail {

template< std::size_t Size >
struct endian_size_type;
template< > struct endian_size_type< 1 > { typedef std::uint8_t type; };
template< > struct endian_size_type< 2 > { typedef std::uint16_t type; };
template< > struct endian_size_type< 4 > { typedef std::uint32_t type; };
template< > struct endian_size_type< 8 > { typedef std::uint64_t type; };

template< std::size_t Size >
inline constexpr typename endian_size_type< Size >::type
endian_swap( typename endian_size_type< Size >::type value ) noexcept
{
	return LIBQ_SWAP_ANY( value, Size );
}

template< int Size, bool Swap = true >
inline void endian_swap( void* to, const void* from ) noexcept
{
	typedef typename endian_size_type< Size >::type size_type;

	if ( Swap )
		*( size_type* )to = endian_swap< Size >( *( size_type* )from );
	else
		*( size_type* )to = *( size_type* )from;
}

template< bool Big, std::size_t Size >
inline void ensure_endian( void* to, const void* from ) noexcept
{
#ifdef LIBQ_LITTLE_ENDIAN
	endian_swap< Size, Big >( to, from );
#else
	endian_swap< Size, !Big >( to, from );
#endif
}

} // namespace detail

template< bool Big, typename T >
class endian
{
public:
	endian( ) noexcept
	: endian( T( ) )
	{ }

	endian( T t ) noexcept
	{
		detail::ensure_endian< Big, sizeof( T ) >(
			reinterpret_cast< void* >( t_ ),
			reinterpret_cast< const void* >( &t ) );
	}

	operator T( ) const noexcept
	{
		T t;
		detail::ensure_endian< Big, sizeof( T ) >(
			reinterpret_cast< void* >( &t ),
			reinterpret_cast< const void* >( t_ ) );
		return t;
	}

	void store( T t ) noexcept
	{
		detail::ensure_endian< Big, sizeof( T ) >(
			reinterpret_cast< void* >( t_ ),
			reinterpret_cast< const void* >( &t ) );
	}

private:
	uint8_t t_[ sizeof( T ) ];
};

template< typename T >
class le
: public endian< false, T >
{
	using endian< false, T >::endian;
};

template< typename T >
class be
: public endian< true, T >
{
	using endian< true, T >::endian;
};

} // namespace q

#endif // LIBQ_ENDIAN_HPP
