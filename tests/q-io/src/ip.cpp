
#include <q-io/ip.hpp>

#include "q-io-test.hpp"

#include <vector>

TEST( ip, ipv4 )
{
	q::io::ipv4_address ipv4_lo( "127.0.0.1" );
	q::io::ipv4_address ipv4_goo( "8.8.8.8" );

	EXPECT_TRUE( ipv4_lo.valid );
	EXPECT_EQ( ipv4_lo.string( ), "127.0.0.1" );
	EXPECT_TRUE( ipv4_goo.valid );
	EXPECT_EQ( ipv4_goo.string( ), "8.8.8.8" );
}

TEST( ip, ipv4_invalid )
{
	std::vector< std::string > invalids{
		"",
		"0",
		"1",
		"127.0.0.0.0",
		"127.0.256.1",
		"::1"
	};

	for ( auto& invalid : invalids )
	{
		auto addr = q::io::ipv4_address::from( invalid );
		EXPECT_FALSE( addr.valid );
	}
}

TEST( ip, ipv4_operators )
{
	q::io::ipv4_address ipv4_a( "8.8.8.8" );
	q::io::ipv4_address ipv4_b( "127.0.0.1" );

	EXPECT_EQ( ipv4_a, ipv4_a );
	EXPECT_NE( ipv4_a, ipv4_b );
	EXPECT_LT( ipv4_a, ipv4_b );

	EXPECT_EQ(
		std::hash< q::io::ipv4_address >( )( ipv4_a ),
		std::hash< q::io::ipv4_address >( )( ipv4_a )
	);
	EXPECT_NE(
		std::hash< q::io::ipv4_address >( )( ipv4_a ),
		std::hash< q::io::ipv4_address >( )( ipv4_b )
	);
}

TEST( ip, ipv6 )
{
	q::io::ipv6_address ipv6_lo( "::1" );
	q::io::ipv6_address ipv6_goo( "2001:4860:4860::8888" );
	q::io::ipv6_address ipv6_zero( "::" );

	EXPECT_TRUE( ipv6_lo.valid );
	EXPECT_EQ( ipv6_lo.string( ), "::1" );
	EXPECT_TRUE( ipv6_goo.valid );
	EXPECT_EQ( ipv6_goo.string( ), "2001:4860:4860::8888" );
	EXPECT_TRUE( ipv6_zero.valid );
	EXPECT_EQ( ipv6_zero.string( ), "::" );
}

TEST( ip, ipv6_invalid )
{
	std::vector< std::string > invalids{
		"",
		"0",
		"1",
		"12345::1",
		"::-1",
		"1::2::3",
		"1:2:3:4:5:6:7",
		"1:2:3:4:5:6:7:8:9"
	};

	for ( auto& invalid : invalids )
	{
		auto addr = q::io::ipv6_address::from( invalid );
		EXPECT_FALSE( addr.valid );
	}
}

TEST( ip, ipv6_operators )
{
	q::io::ipv6_address ipv6_a( "::1" );
	q::io::ipv6_address ipv6_b( "2001:4860:4860::8888" );

	EXPECT_EQ( ipv6_a, ipv6_a );
	EXPECT_NE( ipv6_a, ipv6_b );
	EXPECT_LT( ipv6_a, ipv6_b );

	EXPECT_EQ(
		std::hash< q::io::ipv6_address >( )( ipv6_a ),
		std::hash< q::io::ipv6_address >( )( ipv6_a )
	);
	EXPECT_NE(
		std::hash< q::io::ipv6_address >( )( ipv6_a ),
		std::hash< q::io::ipv6_address >( )( ipv6_b )
	);
}

TEST( ip, ip_valid )
{
	q::io::ip_address ip_empty;
	q::io::ip_address ip_v4( q::io::ipv4_address( "127.0.0.1" ) );
	q::io::ip_address ip_v6( q::io::ipv6_address( "::1" ) );

	EXPECT_FALSE( ip_empty );
	EXPECT_TRUE( ip_v4 );
	EXPECT_TRUE( ip_v6 );
}

TEST( ip, ip_from_v4 )
{
	q::io::ipv4_address ipv4( "8.8.8.8" );
	q::io::ipv4_address ipv4_other( "127.0.0.1" );

	q::io::ip_address ip( ipv4 );

	EXPECT_TRUE( ip );
	EXPECT_TRUE( ip.is_v4( ) );

	const auto& ip_as_v4 = ip.ipv4( );

	EXPECT_EQ( ip_as_v4, ipv4 );
	EXPECT_NE( ip_as_v4, ipv4_other );
	EXPECT_NE( ip, q::io::ip_address( ipv4_other ) );
}

TEST( ip, ip_from_v6 )
{
	q::io::ipv6_address ipv6( "2001:4860:4860::8888" );
	q::io::ipv6_address ipv6_other( "::1" );

	q::io::ip_address ip( ipv6 );

	EXPECT_TRUE( ip );
	EXPECT_TRUE( ip.is_v6( ) );

	const auto& ip_as_v6 = ip.ipv6( );

	EXPECT_EQ( ip_as_v6, ipv6 );
	EXPECT_NE( ip_as_v6, ipv6_other );
	EXPECT_NE( ip, q::io::ip_address( ipv6_other ) );
}


TEST( ip, ip_operators )
{
	q::io::ipv4_address ipv4_a( "8.8.8.8" );
	q::io::ipv4_address ipv4_b( "127.0.0.1" );

	q::io::ipv6_address ipv6_a( "::1" );
	q::io::ipv6_address ipv6_b( "2001:4860:4860::8888" );

	q::io::ip_address ip_4a( ipv4_a );
	q::io::ip_address ip_4b( ipv4_b );
	q::io::ip_address ip_6a( ipv6_a );
	q::io::ip_address ip_6b( ipv6_b );

	EXPECT_EQ( ip_4a, ip_4a );
	EXPECT_NE( ip_4a, ip_4b );
	EXPECT_LT( ip_4a, ip_4b );

	EXPECT_EQ( ip_6a, ip_6a );
	EXPECT_NE( ip_6a, ip_6b );
	EXPECT_LT( ip_6a, ip_6b );

	EXPECT_EQ(
		std::hash< q::io::ip_address >( )( ip_4a ),
		std::hash< q::io::ip_address >( )( ip_4a )
	);
	EXPECT_NE(
		std::hash< q::io::ip_address >( )( ip_4a ),
		std::hash< q::io::ip_address >( )( ip_4b )
	);

	EXPECT_EQ(
		std::hash< q::io::ip_address >( )( ip_6a ),
		std::hash< q::io::ip_address >( )( ip_6a )
	);
	EXPECT_NE(
		std::hash< q::io::ip_address >( )( ip_6a ),
		std::hash< q::io::ip_address >( )( ip_6b )
	);
}

TEST( ip, ip_from )
{
	auto ip_invalid_a = q::io::ip_address::from( "" );
	auto ip_invalid_b = q::io::ip_address::from( "foo" );

	auto ipv4_a = q::io::ip_address::from( "8.8.8.8" );
	auto ipv4_b = q::io::ip_address::from( "127.0.0.1" );

	auto ipv6_a = q::io::ip_address::from( "::1" );
	auto ipv6_b = q::io::ip_address::from( "2001:4860:4860::8888" );

	EXPECT_FALSE( ip_invalid_a );
	EXPECT_FALSE( ip_invalid_b );
	EXPECT_TRUE( ipv4_a );
	EXPECT_TRUE( ipv4_a.is_v4( ) );
	EXPECT_TRUE( ipv4_b );
	EXPECT_TRUE( ipv4_b.is_v4( ) );
	EXPECT_TRUE( ipv6_a );
	EXPECT_TRUE( ipv6_a.is_v6( ) );
	EXPECT_TRUE( ipv6_b );
	EXPECT_TRUE( ipv6_b.is_v6( ) );

	EXPECT_EQ( ipv4_a.string( ), "8.8.8.8" );
	EXPECT_EQ( ipv4_b.string( ), "127.0.0.1" );
	EXPECT_EQ( ipv6_a.string( ), "::1" );
	EXPECT_EQ( ipv6_b.string( ), "2001:4860:4860::8888" );
}
