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

#include <q/exception/exception.hpp>

#include <unordered_map>
#include <vector>
#include <sstream>

#include <errno.h>
#include <string.h>

#if defined( ELAST ) && ELAST < 8192
#	define Q__LAST_ERRNO ELAST
#else
#	define Q__LAST_ERRNO 192
#endif

#ifdef LIBQ_ON_WINDOWS
#	define strerror_r( errno, buf, len ) strerror_s( buf, len, errno )
#	define strncpy strcpy_s
#endif

#if defined( LIBQ_ON_WINDOWS ) || \
	( \
		( _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600 ) && \
		!_GNU_SOURCE \
	)
#	define USE_XSI_STRERROR_R
#endif

namespace {

struct errno_thrower
{
	virtual ~errno_thrower( ) { }

	[[noreturn]] virtual void thrower( ) const = 0;
	virtual std::exception_ptr ptr( ) const = 0;
};

template< typename exception >
struct specific_errno_thrower
: errno_thrower
{
	[[noreturn]] void thrower( ) const override
	{
		Q_THROW( exception( ) );
	}
	std::exception_ptr ptr( ) const override
	{
		return std::make_exception_ptr( exception( ) );
	}
};

struct errno_map
{
	errno_map( );

	std::unordered_map<
		int, std::unique_ptr< errno_thrower >
	> map;

	std::vector<
		std::pair< int, std::unique_ptr< errno_thrower > >
	> pairs;
};

errno_map* get_errno_map( )
{
	static errno_map map;
	return &map;
}

int register_errno_handler(
	int errno_, std::unique_ptr< errno_thrower > thrower )
{
	get_errno_map( )->pairs.emplace_back(
		std::make_pair( errno_, std::move( thrower ) ) );
	return 0;
}

} // anonymous namespace


#define Q__DEFINE_ERRNO_EXCEPTION_IMPL_( Errno, Name ) \
	namespace detail { \
		struct init_errno_handler_ ## Name \
		{ \
			typedef specific_errno_thrower< ::q:: Name > exc; \
			init_errno_handler_ ## Name( ) \
			{ \
				register_errno_handler( \
					Errno, \
					::q::make_unique< exc >( ) \
				); \
			} \
		} _init_errno_handler_ ## Name; \
	} // namespace detail

#include <q/exception/exception_errno.hpp>
#include <q/lib.hpp>

errno_map::errno_map( )
: map( 0 )
{
	pairs.reserve( Q__LAST_ERRNO );

	::q::detail::register_internal_initializer( [ this ]( )
	{
		map.reserve( pairs.size( ) );
		for ( auto& pair : pairs )
			map.insert( std::move( pair ) );
	} );
}


namespace q {

std::exception_ptr get_exception_by_errno( int errno_ )
{
	try
	{
		throw_by_errno( errno_ );
	}
	catch ( ... )
	{
		return std::current_exception( );
	}
}

[[noreturn]] void throw_by_errno( int errno_ )
{
	auto iter = get_errno_map( )->map.find( errno_ );
	if ( iter != get_errno_map( )->map.end( ) )
		iter->second->thrower( );

#	define ERRMSG_MAXLEN 256
	char buf[ ERRMSG_MAXLEN ] = { 0 };
	std::stringstream sstream;

#	ifdef USE_XSI_STRERROR_R
	// XSI-compliant strerror_r
	if ( ::strerror_r( errno_, buf, ERRMSG_MAXLEN ) )
		::strncpy( buf, ERRMSG_MAXLEN, "Unknown error" );
#	else
	// GNU-specific strerror_r
	ignore_result( ::strerror_r( errno_, buf, ERRMSG_MAXLEN ) );
#	endif

	sstream << "errno " << errno_ << ": " << buf;

	Q_THROW( ( errno_exception( ) ), sstream.str( ) );
}

} // namespace q
