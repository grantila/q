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
	ipv4_address( ) : valid( false ) { }
	ipv4_address( ipv4_address&& ) = default;
	ipv4_address( const ipv4_address& ) = default;
	ipv4_address( struct sockaddr* addr );
	ipv4_address( struct sockaddr_in* addr )
	: ipv4_address( reinterpret_cast< struct sockaddr* >( addr ) ) { }
	ipv4_address( const char* addr );
	explicit ipv4_address( const std::string& addr )
	: ipv4_address( addr.c_str( ) )
	{ }

	std::string string( ) const;

	void populate( ::sockaddr_in& addr, std::uint16_t port ) const;

	// These are the same as the constructors, but they won't throw,
	// instead `valid` will be false.
	static ipv4_address from( const char* addr );
	static ipv4_address from( const std::string& addr )
	{
		return from( addr.c_str( ) );
	}

	std::uint8_t data[ 4 ];
	bool valid;
};

struct ipv6_address
{
	ipv6_address( ) : valid( false ) { }
	ipv6_address( ipv6_address&& ) = default;
	ipv6_address( const ipv6_address& ) = default;
	ipv6_address( struct sockaddr* addr );
	ipv6_address( struct sockaddr_in6* addr )
	: ipv6_address( reinterpret_cast< struct sockaddr* >( addr ) ) { }
	ipv6_address( const char* addr );
	explicit ipv6_address( const std::string& addr )
	: ipv6_address( addr.c_str( ) )
	{ }

	std::string string( ) const;

	void populate( ::sockaddr_in6& addr, std::uint16_t port ) const;

	// These are the same as the constructors, but they won't throw,
	// instead `valid` will be false.
	static ipv6_address from( const char* addr );
	static ipv6_address from( const std::string& addr )
	{
		return from( addr.c_str( ) );
	}

	q::be< std::uint16_t > data[ 8 ];
	bool valid;
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
	: invalid( 0 )
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
	std::size_t invalid;

	class iterator
	: public virtual std::iterator<
		std::input_iterator_tag,
		std::shared_ptr< ::sockaddr >
	>
	{
	public:
		iterator( );
		iterator( const iterator& ) = default;
		iterator( iterator&& ) = default;
		~iterator( );

		iterator& operator=( const iterator& ) = default;
		iterator& operator=( iterator&& ) = default;

		iterator operator++( int );
		iterator& operator++( );
		value_type operator*( );
		pointer operator->( );
		bool operator==( const iterator& other );
		bool operator!=( const iterator& other );

	private:
		friend struct ip_addresses;

		iterator(
			ip_addresses* root,
			std::uint16_t port,
			std::size_t ipv4_pos,
			std::size_t ipv6_pos
		);

		void increase( );
		void prepare( );

		ip_addresses* root_;
		std::uint16_t port_;
		std::size_t ipv4_pos_;
		std::size_t ipv6_pos_;

		std::size_t tmp_ipv4_pos_;
		std::size_t tmp_ipv6_pos_;
		std::shared_ptr< sockaddr > tmp_;
	};

	iterator begin( std::uint16_t port );
	iterator end( std::uint16_t port );

private:
	template< typename First, typename... Ips >
	typename std::enable_if<
		q::is_convertible_to_t<
			typename std::decay< First >::type,
			std::string
		>::value
	>::type
	_add( First&& ip, Ips&&... ips )
	{
		if ( detail::might_be_ipv6_address( ip ) )
		{
			ipv6_address addr = ipv6_address::from(
				std::forward< First >( ip ) );
			if ( addr.valid )
				ipv6.push_back( std::move( addr ) );
			else
				++invalid;
		}
		else
		{
			ipv4_address addr = ipv4_address::from(
				std::forward< First >( ip ) );
			if ( addr.valid )
				ipv4.push_back( std::move( addr ) );
			else
				++invalid;
		}

		add( std::forward< Ips >( ips )... );
	}

	template< typename First, typename... Ips >
	typename std::enable_if<
		q::is_same_type<
			typename std::decay< First >::type,
			ipv4_address
		>::value
	>::type
	_add( First&& ip, Ips&&... ips )
	{
		ipv4.push_back( std::forward< First >( ip ) );

		add( std::forward< Ips >( ips )... );
	}

	template< typename First, typename... Ips >
	typename std::enable_if<
		q::is_same_type<
			typename std::decay< First >::type,
			ipv6_address
		>::value
	>::type
	_add( First&& ip, Ips&&... ips )
	{
		ipv6.push_back( std::forward< First >( ip ) );

		add( std::forward< Ips >( ips )... );
	}

	void _add( )
	{ }
};

} } // namespace io, namespace q

#endif // LIBQIO_IP_HPP
