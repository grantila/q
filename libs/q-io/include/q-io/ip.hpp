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

struct ip_addresses;

Q_MAKE_SIMPLE_EXCEPTION( invalid_ip_address );

struct ipv4_address
{
	ipv4_address( ) : valid( false ) { }
	ipv4_address( ipv4_address&& ) = default;
	ipv4_address( const ipv4_address& ) = default;
	ipv4_address( const struct sockaddr* addr );
	ipv4_address( const struct sockaddr_in* addr )
	: ipv4_address( reinterpret_cast< const struct sockaddr* >( addr ) ) { }
	ipv4_address( const char* addr );
	explicit ipv4_address( const std::string& addr )
	: ipv4_address( addr.c_str( ) )
	{ }

	ipv4_address& operator=( ipv4_address&& ) = default;
	ipv4_address& operator=( const ipv4_address& ) = default;

	bool operator==( const ipv4_address& other ) const;
	bool operator!=( const ipv4_address& other ) const;
	bool operator<( const ipv4_address& other ) const;

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
	ipv6_address( const struct sockaddr* addr );
	ipv6_address( const struct sockaddr_in6* addr )
	: ipv6_address( reinterpret_cast< const struct sockaddr* >( addr ) ) { }
	ipv6_address( const char* addr );
	explicit ipv6_address( const std::string& addr )
	: ipv6_address( addr.c_str( ) )
	{ }

	ipv6_address& operator=( ipv6_address&& ) = default;
	ipv6_address& operator=( const ipv6_address& ) = default;

	bool operator==( const ipv6_address& other ) const;
	bool operator!=( const ipv6_address& other ) const;
	bool operator<( const ipv6_address& other ) const;

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

class ip_address
{
public:
	ip_address( );
	ip_address( ipv4_address addr );
	ip_address( ipv6_address addr );
	ip_address( const ip_address& );
	ip_address( ip_address&& );
	ip_address( const struct sockaddr* addr );
	ip_address( const struct sockaddr_in* addr )
	: ip_address( reinterpret_cast< const struct sockaddr* >( addr ) ) { }
	ip_address( const struct sockaddr_in6* addr )
	: ip_address( reinterpret_cast< const struct sockaddr* >( addr ) ) { }
	ip_address( const char* addr );
	explicit ip_address( const std::string& addr )
	: ip_address( addr.c_str( ) )
	{ }
	~ip_address( );

	ip_address& operator=( ipv4_address addr );
	ip_address& operator=( ipv6_address addr );
	ip_address& operator=( const ip_address& );
	ip_address& operator=( ip_address&& );

	bool operator==( const ip_address& other ) const;
	bool operator!=( const ip_address& other ) const;
	bool operator<( const ip_address& other ) const;

	bool is_v4( ) const;
	bool is_v6( ) const;

	const ipv4_address& ipv4( ) const;
	const ipv6_address& ipv6( ) const;

	operator bool( ) const;

	std::string string( ) const;

	void populate( ::sockaddr_in& addr, std::uint16_t port ) const;
	void populate( ::sockaddr_in6& addr, std::uint16_t port ) const;
	std::shared_ptr< ::sockaddr > get_sockaddr( std::uint16_t port ) const;

	// These are the same as the constructors, but they won't throw,
	// instead the ip_address will not be "valid" (operator bool will
	// return false), as if it was default constructed.
	static ip_address from( const char* addr );
	static ip_address from( const std::string& addr )
	{
		return from( addr.c_str( ) );
	}

private:
	friend struct ip_addresses;

	void _clear( );

	enum class state_type
	{
		uninitialized,
		ipv4,
		ipv6
	};
	state_type state_;

	union
	{
		ipv4_address ipv4_;
		ipv6_address ipv6_;
	};
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

	std::vector< ip_address > ips;
	std::size_t invalid;

	class iterator
	: public virtual std::iterator<
		std::input_iterator_tag,
		ip_address
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

		iterator( ip_addresses* root, std::size_t pos );

		void increase( );
		void prepare( );

		ip_addresses* root_;
		std::size_t pos_;

		std::size_t tmp_pos_;
		ip_address tmp_;
	};

	iterator begin( );
	iterator end( );

private:
	template< typename First, typename... Ips >
	typename std::enable_if<
		q::is_convertible_to_t<
			typename std::decay< First >::type,
			std::string
		>::value
	>::type
	_add( First&& ip, Ips&&... rest )
	{
		if ( detail::might_be_ipv6_address( ip ) )
		{
			ipv6_address addr = ipv6_address::from(
				std::forward< First >( ip ) );
			if ( addr.valid )
				ips.emplace_back( std::move( addr ) );
			else
				++invalid;
		}
		else
		{
			ipv4_address addr = ipv4_address::from(
				std::forward< First >( ip ) );
			if ( addr.valid )
				ips.emplace_back( std::move( addr ) );
			else
				++invalid;
		}

		add( std::forward< Ips >( rest )... );
	}

	template< typename First, typename... Ips >
	typename std::enable_if<
		q::is_same_type<
			typename std::decay< First >::type,
			ip_address
		>::value
	>::type
	_add( First&& ip, Ips&&... rest )
	{
		if ( ip )
			ips.push_back( std::forward< First >( ip ) );
		else
			++invalid;

		add( std::forward< Ips >( rest )... );
	}

	template< typename First, typename... Ips >
	typename std::enable_if<
		q::is_same_type<
			typename std::decay< First >::type,
			ipv4_address
		>::value
	>::type
	_add( First&& ip, Ips&&... rest )
	{
		ips.emplace_back( std::forward< First >( ip ) );

		add( std::forward< Ips >( rest )... );
	}

	template< typename First, typename... Ips >
	typename std::enable_if<
		q::is_same_type<
			typename std::decay< First >::type,
			ipv6_address
		>::value
	>::type
	_add( First&& ip, Ips&&... rest )
	{
		ips.emplace_back( std::forward< First >( ip ) );

		add( std::forward< Ips >( rest )... );
	}

	void _add( )
	{ }
};

} } // namespace io, namespace q

namespace std {

template< > struct hash< q::io::ipv4_address >
{
	size_t operator( )( const q::io::ipv4_address& ip ) const;
};

template< > struct hash< q::io::ipv6_address >
{
	size_t operator( )( const q::io::ipv6_address& ip ) const;
};

template< > struct hash< q::io::ip_address >
{
	size_t operator( )( const q::io::ip_address& ip ) const;
};

} // namespace std

#endif // LIBQIO_IP_HPP
