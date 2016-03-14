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

#ifndef LIBQIO_IP_HPP
#define LIBQIO_IP_HPP

#include <q/endian.hpp>
#include <q/exception.hpp>

#include <string>
#include <vector>

#include <netinet/in.h>

namespace q { namespace io {

Q_MAKE_SIMPLE_EXCEPTION( invalid_ip_address );

struct ipv4_address
{
	ipv4_address( ) = default;
	ipv4_address( ipv4_address&& ) = default;
	ipv4_address( const ipv4_address& ) = default;
	ipv4_address( const char* addr );
	explicit ipv4_address( const std::string& addr )
	: ipv4_address( addr.c_str( ) )
	{ }

	std::string string( ) const;

	void populate( ::sockaddr_in& addr, std::uint16_t port ) const;

	std::uint8_t data[ 4 ];
};

struct ipv6_address
{
	ipv6_address( ) = default;
	ipv6_address( ipv6_address&& ) = default;
	ipv6_address( const ipv6_address& ) = default;
	ipv6_address( const char* addr );
	explicit ipv6_address( const std::string& addr )
	: ipv6_address( addr.c_str( ) )
	{ }

	std::string string( ) const;

	void populate( ::sockaddr_in6& addr, std::uint16_t port ) const;

	q::be< std::uint16_t > data[ 8 ];
};

namespace detail {

static inline bool might_be_ipv6_address( const char* s )
{
	return !!::strchr( s, ':' );
}
static inline bool might_be_ipv6_address( const std::string& s )
{
	return s.find( ':' ) != std::string::npos;
}

} // namespace detail

struct ip_addresses
{
	template< typename... Ips >
	ip_addresses( Ips&&... ips )
	{
		_add( std::forward< Ips >( ips )... );
	}

	template< typename... Ips >
	void add( Ips&&... ips )
	{
		_add( std::forward< Ips >( ips )... );
	}

	std::vector< ipv4_address > ipv4;
	std::vector< ipv6_address > ipv6;

private:
	template< typename First, typename... Ips >
	typename std::enable_if<
		q::is_convertible_to<
			typename std::decay< First >::type,
			std::string
		>::value
	>::type
	_add( First&& ip, Ips&&... ips )
	{
		if ( detail::might_be_ipv6_address( ip ) )
			ipv6.push_back(
				ipv6_address( std::forward< First >( ip ) ) );
		else
			ipv4.push_back(
				ipv4_address( std::forward< First >( ip ) ) );

		add( std::forward< Ips >( ips )... );
	}

	void _add( )
	{ }
};

} } // namespace io, namespace q

#endif // LIBQIO_IP_HPP
