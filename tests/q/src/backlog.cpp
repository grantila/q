
#include <q/backlog.hpp>

#include <q-test/q-test.hpp>

TEST( backlog, infinity )
{
	q::backlog bl( q::backlog::infinity );
	EXPECT_TRUE( bl.is_infinity( ) );
}

TEST( backlog, default_value )
{
	q::backlog bl;
	EXPECT_EQ( bl.get( ), 1 );
	EXPECT_EQ( bl.get( 5 ), 5 );
	EXPECT_FALSE( bl.is_infinity( ) );
}

TEST( backlog, other_value )
{
	q::backlog bl( 10 );
	EXPECT_EQ( bl.get( ), 10 );
	EXPECT_EQ( bl.get( 5 ), 10 );
	EXPECT_FALSE( bl.is_infinity( ) );
}
