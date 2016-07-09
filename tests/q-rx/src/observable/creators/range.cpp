
#include "../../q-rx-test.hpp"

Q_TEST_MAKE_SCOPE( ob_create_range );

TEST_F( ob_create_range, void_empty )
{
	auto o_range = q::rx::observable< void >::range( 0, { queue } );

	run( o_range.consume( EXPECT_NO_CALL_WRAPPER( [ ]( ) { } ) ) );
}

TEST_F( ob_create_range, void_non_empty )
{
	auto o_range = q::rx::observable< void >::range( 3, { queue } );

	run( o_range.consume( EXPECT_N_CALLS_WRAPPER( 3, [ ]( ) { } ) ) );
}

TEST_F( ob_create_range, void_empty_with_backlog )
{
	auto o_range = q::rx::observable< void >::range(
		0, { queue, q::rx::backlog( 1 ) } );

	run( o_range.consume( EXPECT_NO_CALL_WRAPPER( [ ]( ) { } ) ) );
}

TEST_F( ob_create_range, void_non_empty_with_backlog )
{
	auto o_range = q::rx::observable< void >::range(
		3, { queue, q::rx::backlog( 2 ) } );

	run( o_range.consume( EXPECT_N_CALLS_WRAPPER( 3, [ ]( ) { } ) ) );
}

TEST_F( ob_create_range, int_empty )
{
	auto o_range = q::rx::observable< int >::range( 1, 0, { queue } );

	run( o_range.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ) );
}

TEST_F( ob_create_range, int_non_empty )
{
	auto o_range = q::rx::observable< int >::range( 1, 3, { queue } );

	int counter = 1;

	auto consumer = EXPECT_N_CALLS_WRAPPER( 3, [ &counter ]( int value )
	{
		EXPECT_EQ( value, counter );
		++counter;
	} );

	run( o_range.consume( consumer ) );
}

TEST_F( ob_create_range, int_empty_with_backlog )
{
	auto o_range = q::rx::observable< int >::range(
		1, 0, { queue, q::rx::backlog( 1 ) } );

	run( o_range.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ) );
}

TEST_F( ob_create_range, int_non_empty_with_backlog )
{
	auto o_range = q::rx::observable< int >::range(
		1, 3, { queue, q::rx::backlog( 1 ) } );

	int counter = 1;

	auto consumer = EXPECT_N_CALLS_WRAPPER( 3, [ &counter ]( int value )
	{
		EXPECT_EQ( value, counter );
		++counter;
	} );

	run( o_range.consume( consumer ) );
}

TEST_F( ob_create_range, iterator_non_empty_with_backlog )
{
	std::vector< std::string > strings{ "a", "b", "c" };
	typedef std::vector< std::string >::iterator iter_type;

	auto o_range = q::rx::observable< iter_type >::range(
		strings.begin( ), 3, { queue, q::rx::backlog( 2 ) } );

	int counter = 0;

	auto consumer = [ &strings, &counter ]( iter_type value )
	{
		EXPECT_EQ( *value, strings[ counter ] );
		++counter;
	};

	run( o_range.consume( EXPECT_N_CALLS_WRAPPER( 3, consumer ) ) );
}
