
#include <q-io/dns.hpp>

#include "q-io-test.hpp"

QIO_TEST_MAKE_SCOPE( dns );

// These unit tests go for www.google.com, and we expect it resolves to at
// least one IPv4 and one IPv6 address.

TEST_F( dns, lookup_simple )
{
	auto lookup = io_dispatcher->lookup( "www.google.com" )
	.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::io::resolver_response&& response )
		{
			EXPECT_GE( response.ips.ipv4.size( ), 1 );
			EXPECT_GE( response.ips.ipv6.size( ), 1 );
		}
	) );

	run( std::move( lookup ) );
}

TEST_F( dns, lookup_ipv4 )
{
	auto resolver = q::make_shared< q::io::resolver >( io_dispatcher );

	auto flags = q::io::resolver::resolve_flags::ipv4;

	auto lookup = resolver->lookup( queue, "www.google.com", flags )
	.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::io::resolver_response&& response )
		{
			EXPECT_GE( response.ips.ipv4.size( ), 1 );
			EXPECT_EQ( response.ips.ipv6.size( ), 0 );
		}
	) );

	run( std::move( lookup ) );
}

TEST_F( dns, lookup_ipv6 )
{
	auto resolver = q::make_shared< q::io::resolver >( io_dispatcher );

	auto flags = q::io::resolver::resolve_flags::ipv6;

	auto lookup = resolver->lookup( queue, "www.google.com", flags )
	.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::io::resolver_response&& response )
		{
			EXPECT_EQ( response.ips.ipv4.size( ), 0 );
			EXPECT_GE( response.ips.ipv6.size( ), 1 );
		}
	) );

	run( std::move( lookup ) );
}

TEST_F( dns, lookup_normal )
{
	auto resolver = q::make_shared< q::io::resolver >( io_dispatcher );

	auto flags =
		q::io::resolver::resolve_flags::ipv4 |
		q::io::resolver::resolve_flags::ipv6;

	auto lookup = resolver->lookup( queue, "www.google.com", flags )
	.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::io::resolver_response&& response )
		{
			EXPECT_GE( response.ips.ipv4.size( ), 1 );
			EXPECT_GE( response.ips.ipv6.size( ), 1 );
		}
	) );

	run( std::move( lookup ) );
}
