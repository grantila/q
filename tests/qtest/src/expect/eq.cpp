
#include <q-test/expect.hpp>

Q_TEST_MAKE_SCOPE( expect_eq );

TEST_F( expect_eq, async_integer_eq_integer )
{
	auto p = q::with( queue, 6 ).share( );

	EVENTUALLY_EXPECT_EQ( p, 6 );
}

TEST_F( expect_eq, async_integers_eq_integers )
{
	auto p = q::with( queue, 5, 6 ).share( );

	EVENTUALLY_EXPECT_EQ( p, std::make_tuple( 5, 6 ) );
}

TEST_F( expect_eq, integer_eq_integer )
{
	EVENTUALLY_EXPECT_EQ( 6, 6 );
}

TEST_F( expect_eq, integers_eq_integers )
{
	EVENTUALLY_EXPECT_EQ(
		std::make_tuple( 5, 6 ), std::make_tuple( 5, 6 ) );
}

TEST_F( expect_eq, integer_eq_async_integer )
{
	auto p = q::with( queue, 6 ).share( );

	EVENTUALLY_EXPECT_EQ( 6, p );
}

TEST_F( expect_eq, integers_eq_async_integers )
{
	auto p = q::with( queue, 5, 6 ).share( );

	EVENTUALLY_EXPECT_EQ( std::make_tuple( 5, 6 ), p );
}

TEST_F( expect_eq, async_integer_eq_async_integer )
{
	auto p1 = q::with( queue, 6 ).share( );
	auto p2 = q::with( queue, 6 ).share( );

	EVENTUALLY_EXPECT_EQ( p1, p2 );
}


TEST_F( expect_eq, async_string_eq_string )
{
	auto p = q::with( queue, std::string( "a" ) ).share( );

	EVENTUALLY_EXPECT_EQ( p, std::string( "a" ) );
}

TEST_F( expect_eq, string_eq_string )
{
	EVENTUALLY_EXPECT_EQ( std::string( "a" ), std::string( "a" ) );
}

TEST_F( expect_eq, string_eq_async_string )
{
	auto p = q::with( queue, std::string( "a" ) ).share( );

	EVENTUALLY_EXPECT_EQ( std::string( "a" ), p );
}

TEST_F( expect_eq, async_string_eq_async_string )
{
	auto p1 = q::with( queue, std::string( "a" ) ).share( );
	auto p2 = q::with( queue, std::string( "a" ) ).share( );

	EVENTUALLY_EXPECT_EQ( p1, p2 );
}


TEST_F( expect_eq, async_charp_eq_charp )
{
	auto p = q::with( queue, "a" ).share( );

	EVENTUALLY_EXPECT_EQ( p, "a" );
}

TEST_F( expect_eq, charp_eq_charp )
{
	EVENTUALLY_EXPECT_EQ( "a", "a" );
}

TEST_F( expect_eq, charp_eq_async_charp )
{
	auto p = q::with( queue, "a" ).share( );

	EVENTUALLY_EXPECT_EQ( "a", p );
}

TEST_F( expect_eq, async_charp_eq_async_charp )
{
	auto p1 = q::with( queue, "a" ).share( );
	auto p2 = q::with( queue, "a" ).share( );

	EVENTUALLY_EXPECT_EQ( p1, p2 );
}


TEST_F( expect_eq, async_charp_eq_string )
{
	auto p = q::with( queue, "a" ).share( );

	EVENTUALLY_EXPECT_EQ( p, std::string( "a" ) );
}

TEST_F( expect_eq, charp_eq_string )
{
	EVENTUALLY_EXPECT_EQ( "a", std::string( "a" ) );
}

TEST_F( expect_eq, charp_eq_async_string )
{
	auto p = q::with( queue, std::string( "a" ) ).share( );

	EVENTUALLY_EXPECT_EQ( "a", p );
}

TEST_F( expect_eq, async_charp_eq_async_string )
{
	auto p1 = q::with( queue, "a" ).share( );
	auto p2 = q::with( queue, std::string( "a" ) ).share( );

	EVENTUALLY_EXPECT_EQ( p1, p2 );
}


TEST_F( expect_eq, async_string_eq_charp )
{
	auto p = q::with( queue, std::string( "a" ) ).share( );

	EVENTUALLY_EXPECT_EQ( p, "a" );
}

TEST_F( expect_eq, string_eq_charp )
{
	EVENTUALLY_EXPECT_EQ( std::string( "a" ), "a" );
}

TEST_F( expect_eq, string_eq_async_charp )
{
	auto p = q::with( queue, "a" ).share( );

	EVENTUALLY_EXPECT_EQ( std::string( "a" ), p );
}

TEST_F( expect_eq, string_charp_eq_async_charp )
{
	auto p1 = q::with( queue, std::string( "a" ) ).share( );
	auto p2 = q::with( queue, "a" ).share( );

	EVENTUALLY_EXPECT_EQ( p1, p2 );
}
