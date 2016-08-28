
#include "../../q-rx-test.hpp"

Q_TEST_MAKE_SCOPE( ob_create_repeat );

TEST_F( ob_create_repeat, void_empty )
{
	auto o_repeat = q::rx::observable< void >::range( 0, { queue } )
	.repeat( 1 );

	run( o_repeat.consume( EXPECT_NO_CALL_WRAPPER( [ ]( ) { } ) ) );
}

TEST_F( ob_create_repeat, void_non_empty )
{
	auto o_repeat = q::rx::observable< void >::range( 3, { queue } )
	.repeat( 2 );

	run( o_repeat.consume( EXPECT_N_CALLS_WRAPPER( 6, [ ]( ) { } ) ) );
}

TEST_F( ob_create_repeat, void_empty_with_backlog )
{
	auto o_repeat = q::rx::observable< void >::range( 0, { queue } )
	.repeat( 2, { q::rx::backlog( 1 ) } );

	run( o_repeat.consume( EXPECT_NO_CALL_WRAPPER( [ ]( ) { } ) ) );
}

TEST_F( ob_create_repeat, void_non_empty_with_backlog )
{
	auto o_repeat = q::rx::observable< void >::range( 3, { queue } )
	.repeat( 2, q::rx::backlog( 1 ) );

	run( o_repeat.consume( EXPECT_N_CALLS_WRAPPER( 6, [ ]( ) { } ) ) );
}

TEST_F( ob_create_repeat, int_empty )
{
	auto o_repeat = q::rx::observable< int >::range( 1, 0, { queue } )
	.repeat( 2 );

	run( o_repeat.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ) );
}

TEST_F( ob_create_repeat, int_non_empty )
{
	auto o_repeat = q::rx::observable< int >::range( 1, 3, { queue } )
	.repeat( 2 );

	std::vector< int > expected{ 1, 2, 3, 1, 2, 3 };
	int index = 0;

	auto consumer = [ &expected, &index ]( int value )
	{
		EXPECT_EQ( value, expected[ index ] );
		++index;
	};

	run( o_repeat.consume( EXPECT_N_CALLS_WRAPPER( 6, consumer ) ) );
}

TEST_F( ob_create_repeat, int_empty_with_backlog )
{
	auto o_repeat = q::rx::observable< int >::range( 1, 0, { queue } )
	.repeat( 2, { q::rx::backlog( 1 ) } );

	run( o_repeat.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ) );
}

TEST_F( ob_create_repeat, int_non_empty_with_backlog )
{
	auto o_repeat = q::rx::observable< int >::range( 1, 3, { queue } )
	.repeat( 2, { q::rx::backlog( 1 ) } );

	std::vector< int > expected{ 1, 2, 3, 1, 2, 3 };
	int index = 0;

	auto consumer = [ &expected, &index ]( int value )
	{
		EXPECT_EQ( value, expected[ index ] );
		++index;
	};

	run( o_repeat.consume( EXPECT_N_CALLS_WRAPPER( 6, consumer ) ) );
}

// TODO: Test upstream cancelation in both limited and unlimited repeats
