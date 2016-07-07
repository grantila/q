
#include "../../q-rx-test.hpp"

Q_TEST_MAKE_SCOPE( ob_create_just );

TEST_F( ob_create_just, empty )
{
	auto o_empty = q::rx::observable< int >::just( queue );

	run( o_empty.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ) );
}

TEST_F( ob_create_just, one )
{
	auto o_empty = q::rx::observable< int >::just( queue, 5 );

	auto consumer = EXPECT_N_CALLS_WRAPPER( 1, [ ]( int value )
	{
		EXPECT_EQ( value, 5 );
	} );

	run( o_empty.consume( consumer ) );
}

TEST_F( ob_create_just, three )
{
	auto o_empty = q::rx::observable< int >::just( queue, 1, 2, 3 );

	int counter = 1;

	auto consumer = EXPECT_N_CALLS_WRAPPER( 3, [ &counter ]( int value )
	{
		EXPECT_EQ( value, counter );
		++counter;
	} );

	run( o_empty.consume( consumer ) );
}
