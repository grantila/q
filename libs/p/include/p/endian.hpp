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

#ifndef LIBP_ENDIAN_HPP
#define LIBP_ENDIAN_HPP

#include <q/pp.hpp>

#ifdef LIBQ_ON_LINUX
#	include <byteswap.h>
#	define LIBP_SWAP_16 bswap_16
#	define LIBP_SWAP_32 bswap_32
#	define LIBP_SWAP_64 bswap_64
#else
#	define LIBP_SWAP_CHUNK( chunk, x ) \
		( ( ( x ) >> chunk ) | ( ( x ) << chunk ) )
#	define LIBP_SWAP_16( x ) \
		LIBP_SWAP_CHUNK( 8, x )
#	define LIBP_SWAP_32( x ) \
		LIBP_SWAP_CHUNK( 16, \
			( ( ( x ) << 8 ) & 0xFF00FF00 ) | \
			( ( ( x ) >> 8 ) & 0xFF00FF ) \
		)
#	define LIBP_SWAP_64_HALF( x ) \
			( ( ( x ) << 16 ) & 0xFFFF0000FFFF0000ULL ) | \
			( ( ( x ) >> 16 ) & 0x0000FFFF0000FFFFULL )
#	define LIBP_SWAP_64( x ) \
		LIBP_SWAP_CHUNK( 32, \
			LIBP_SWAP_64_HALF( \
				( ( ( x ) << 8 ) & 0xFF00FF00FF00FF00ULL ) | \
				( ( ( x ) >> 8 ) & 0x00FF00FF00FF00FFULL ) \
			) \
		)
#endif

#define LIBP_SWAP_ANY( x, size ) \
		( size == 1 ) \
		? ( x ) \
		: ( size == 2 ) \
		  ? LIBP_SWAP_16( x ) \
		  : ( size == 4 ) \
		    ? LIBP_SWAP_32( x ) \
		    : ( size == 8 ) \
		      ? LIBP_SWAP_64( x ) \
		      : ( x ) // Error

namespace p {

namespace detail {

template< std::size_t Size >
struct endian_size_type;
template< > struct endian_size_type< 1 > { typedef std::uint8_t type; };
template< > struct endian_size_type< 2 > { typedef std::uint16_t type; };
template< > struct endian_size_type< 4 > { typedef std::uint32_t type; };
template< > struct endian_size_type< 8 > { typedef std::uint64_t type; };

template< typename Size >
inline typename endian_size_type< Size >::type
endian_swap( typename endian_size_type< Size >::type value )
{
	return LIBP_SWAP_ANY( value, Size );
}

template< int Size, bool Swap = true >
inline void endian_swap( void* to, void* from )
{
	typedef typename endian_size_type< Size >::type size_type;

	if ( Swap )
		*( size_type* )to = endian_swap< Size >( *( size_type* )from );
	else
		*( size_type* )to = *( size_type* )from;
}

template< bool Big, int Size >
void ensure_endian( void* to, void* from )
{
#ifdef LIBP_LITTLE_ENDIAN
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
	endian( )
	: endian( T( ) )
	{ }

	endian( T t )
	{
		detail::ensure_endian< Big, sizeof T >( t_, &t );
	}

	operator T( ) const
	{
		T t;
		detail::ensure_endian< Big, sizeof T >( &t, t_ );
		return std::move( t );
	}

	void store( T t )
	{
		detail::ensure_endian< Big, sizeof T >( t_, &t );
	}

private:
	uint8_t t_[ sizeof T ];
};

template< typename T >
class le
: public endian< false, T >
{
	using endian::endian;
};

template< typename T >
class be
: public endian< true, T >
{
	using endian::endian;
};

le< int > li;
be< long > bi;

} // namespace p

#endif // LIBP_ENDIAN_HPP
