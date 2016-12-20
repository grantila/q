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

#include <q-io/ip.hpp>

#include <sstream>
#include <cstdio>

#include <arpa/inet.h>

namespace q { namespace io {

ipv4_address::ipv4_address( const char* addr )
: valid( true )
{
	auto ret = inet_pton( AF_INET, addr, data );

	if ( ret != 1 )
		Q_THROW( invalid_ip_address( ) );
}

ipv4_address::ipv4_address( struct sockaddr* addr )
: valid( true )
{
	if ( addr->sa_family != AF_INET )
		Q_THROW( invalid_ip_address( ) );

	auto addr_in = reinterpret_cast< struct sockaddr_in* >( addr );

	std::memcpy(
		data,
		&addr_in->sin_addr,
		std::min< std::size_t >( 4, sizeof addr_in->sin_addr )
	);
}

std::string ipv4_address::string( ) const
{
	if ( true )
	{
		char buf[ INET_ADDRSTRLEN ];
		auto ret = inet_ntop( AF_INET, data, buf, INET_ADDRSTRLEN );

		if ( !ret )
			Q_THROW( invalid_ip_address( ) );

		return buf;
	}
	else
	{
		// If we don't have support for inet_ntop
		// TODO: Consider removing, we could demand inet_ntop
		std::stringstream s;
		s
			<< (int)data[ 0 ] << "."
			<< (int)data[ 1 ] << "."
			<< (int)data[ 2 ] << "."
			<< (int)data[ 3 ];
		return s.str( );
	}
}

void ipv4_address::populate( ::sockaddr_in& addr, std::uint16_t port ) const
{
	memset( &addr, 0, sizeof addr );

#if defined( LIBQ_ON_OSX ) || defined( LIBQ_ON_BSD )
	addr.sin_len = sizeof addr;
#endif

	addr.sin_family = AF_INET;
	addr.sin_port = htons( port );
	memcpy( &addr.sin_addr, &data[ 0 ], 4 );
}

ipv4_address ipv4_address::from( const char* addr )
{
	ipv4_address ipv4;

	auto ret = inet_pton( AF_INET, addr, ipv4.data );

	if ( ret != 1 )
		std::memset( ipv4.data, 0, sizeof( ipv4.data ) );
	else
		ipv4.valid = true;

	return ipv4;
}


ipv6_address::ipv6_address( const char* addr )
: valid( true )
{
	auto ret = inet_pton( AF_INET6, addr, data );

	if ( ret != 1 )
		Q_THROW( invalid_ip_address( ) );
}

ipv6_address::ipv6_address( struct sockaddr* addr )
: valid( true )
{
	if ( addr->sa_family != AF_INET && addr->sa_family != AF_INET6 )
		Q_THROW( invalid_ip_address( ) );

	if ( addr->sa_len < 16 )
		// This might mean ipv4
		Q_THROW( invalid_ip_address( ) );

	auto addr_in = reinterpret_cast< struct sockaddr_in6* >( addr );

	std::memcpy(
		data,
		&addr_in->sin6_addr,
		std::min< std::size_t >( 16, sizeof addr_in->sin6_addr )
	);
}

static std::string ipv6_address_to_string( const ipv6_address& ipv6 )
{
	// Shortening rules for IPv6 applies here, meaning longest consecutive
	// all-zero field, secondarily the left-most such, is shortened. The
	// rest are left as is. Shortening doesn't apply to a single zero field.
	int shortened_field_start = -1;
	int shortened_field_length = 0;
	int cur_first = -1;
	for ( size_t i = 0; i < 8; ++i )
	{
		if ( ipv6.data[ i ] )
		{
			// Non-null, terminate last
			if ( cur_first != -1 )
			{
				// Record this as the longest series of zero
				// fields if it is.
				if ( i - cur_first > shortened_field_length )
				{
					shortened_field_start = cur_first;
					shortened_field_length = i - cur_first;
				}
				cur_first = -1;
			}
		}
		else
		{
			// Begin or continue zero-field series
			if ( cur_first == -1 )
				cur_first = i;
		}
	}
	if ( cur_first != -1 && 8 - cur_first > shortened_field_length )
	{
		shortened_field_start = cur_first;
		shortened_field_length = 8 - cur_first;
	}

	std::stringstream s;
	s.setf( std::ios::hex, std::ios::basefield );

	for ( size_t i = 0; i < 8; ++i )
	{
		if (
			shortened_field_start == -1 ||
			i < shortened_field_start ||
			i >= ( shortened_field_start + shortened_field_length )
		)
			s << ":" << (std::ptrdiff_t)ipv6.data[ i ];
		else if ( shortened_field_start == i )
			s << ":";
	}

	return s.str( ).substr( 1 );
}

std::string ipv6_address::string( ) const
{
	if ( true )
	{
		char buf[ INET6_ADDRSTRLEN ];
		auto ret = inet_ntop( AF_INET6, data, buf, INET6_ADDRSTRLEN );

		if ( !ret )
			Q_THROW( invalid_ip_address( ) );

		return buf;
	}
	else
	{
		// TODO: Consider removing, we could demand inet_ntop
		return ipv6_address_to_string( *this );
	}
}

void ipv6_address::populate( ::sockaddr_in6& addr, std::uint16_t port ) const
{
	memset( &addr, 0, sizeof addr );

#if defined( LIBQ_ON_OSX ) || defined( LIBQ_ON_BSD )
	addr.sin6_len = sizeof addr;
#endif

	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons( port );
	addr.sin6_flowinfo = 0;
	memcpy( &addr.sin6_addr, &data, sizeof( data ) );
	addr.sin6_scope_id = 0;
}

ipv6_address ipv6_address::from( const char* addr )
{
	ipv6_address ipv6;

	auto ret = inet_pton( AF_INET6, addr, ipv6.data );

	if ( ret != 1 )
		std::memset( ipv6.data, 0, sizeof( ipv6.data ) );
	else
		ipv6.valid = true;

	return ipv6;
}


ip_addresses::iterator::iterator( )
: root_( nullptr )
{
}

ip_addresses::iterator::iterator(
	ip_addresses* root,
	std::uint16_t port,
	std::size_t ipv4_pos,
	std::size_t ipv6_pos
)
: root_( root )
, port_( port )
, ipv4_pos_( ipv4_pos )
, ipv6_pos_( ipv6_pos )
, tmp_ipv4_pos_( ipv4_pos )
, tmp_ipv6_pos_( ipv6_pos )
{
}

ip_addresses::iterator::~iterator( )
{
}

ip_addresses::iterator ip_addresses::iterator::operator++( int )
{
	iterator ret( *this );

	increase( );

	return ret;
}

ip_addresses::iterator& ip_addresses::iterator::operator++( )
{
	increase( );

	return *this;
}

ip_addresses::iterator::value_type ip_addresses::iterator::operator*( )
{
	prepare( );

	return tmp_;
}

ip_addresses::iterator::pointer ip_addresses::iterator::operator->( )
{
	prepare( );

	return &tmp_;
}

bool ip_addresses::iterator::operator==( const ip_addresses::iterator& other )
{
	return root_ == other.root_ &&
		port_ == other.port_ &&
		ipv4_pos_ == other.ipv4_pos_ &&
		ipv6_pos_ == other.ipv6_pos_;
}

bool ip_addresses::iterator::operator!=( const ip_addresses::iterator& other )
{
	return !operator==( other );
}

void ip_addresses::iterator::increase( )
{
	std::size_t ipv4_size = root_->ipv4.size( );

	if ( ipv4_pos_ < ipv4_size )
		++ipv4_pos_;

	if ( ipv4_pos_ < ipv4_size )
		return;

	std::size_t ipv6_size = root_->ipv6.size( );

	if ( ipv6_pos_ < ipv6_size )
		++ipv6_pos_;
}

void ip_addresses::iterator::prepare( )
{
	if (
		tmp_ipv4_pos_ == ipv4_pos_ &&
		tmp_ipv6_pos_ == ipv6_pos_ &&
		!!tmp_
	)
		// Re-use the existing object, it's not dirty
		return;

	tmp_ipv4_pos_ = ipv4_pos_;
	tmp_ipv6_pos_ = ipv6_pos_;

	if ( ipv4_pos_ < root_->ipv4.size( ) )
	{
		const ipv4_address& ipv4_address = root_->ipv4[ ipv4_pos_ ];

		// reinterpret_pointer_cast is C++17, so we need to do this...
		auto addr_in = new sockaddr_in;

		tmp_.reset( reinterpret_cast< sockaddr* >( addr_in ) );

		ipv4_address.populate( *addr_in, port_ );

		return;
	}

	if ( ipv6_pos_ < root_->ipv6.size( ) )
	{
		const ipv6_address& ipv6_address = root_->ipv6[ ipv6_pos_ ];

		// reinterpret_pointer_cast is C++17, so we need to do this...
		auto addr_in6 = new sockaddr_in6;

		tmp_.reset( reinterpret_cast< sockaddr* >( addr_in6 ) );

		ipv6_address.populate( *addr_in6, port_ );

		return;
	}

	tmp_.reset( );
}

ip_addresses::iterator ip_addresses::begin( std::uint16_t port )
{
	return iterator( this, port, 0, 0 );
}

ip_addresses::iterator ip_addresses::end( std::uint16_t port )
{
	return iterator( this, port, ipv4.size( ), ipv6.size( ) );
}

} } // namespace io, namespace q
