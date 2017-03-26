
#include "../core.hpp"

Q_TEST_MAKE_SCOPE( delay );

TEST_F( delay, with_value )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> long
		{
			return 17;
		} ) )
		.delay( std::chrono::nanoseconds( 1 ) )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( long value )
		{
			EXPECT_EQ( 17, value );
		} ) )
	);
}

TEST_F( delay, with_exception )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( )
		{
			Q_THROW( Error( ) );
		} ) )
		.delay( std::chrono::nanoseconds( 1 ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( Error& ) { } ) )
		.then( [ ]( ) { } )
	);
}

TEST_F( delay, create_with_time )
{
	auto now = q::timer::point_type::clock::now( );

	run( q::delay( queue, now + std::chrono::microseconds( 1 ) ) );
}

TEST_F( delay, create_with_duration )
{
	run( q::delay( queue, std::chrono::microseconds( 1 ) ) );
}

TEST_F( delay, create_with_time_and_value )
{
	auto now = q::timer::point_type::clock::now( );

	run(
		q::delay( queue, now + std::chrono::microseconds( 1 ), "hello" )
		.then( EXPECT_CALL_WRAPPER( [ ]( std::string s )
		{
			EXPECT_EQ( s, "hello" );
		} ) )
	);
}

TEST_F( delay, create_with_duration_and_value )
{
	run(
		q::delay( queue, std::chrono::microseconds( 1 ), "world" )
		.then( EXPECT_CALL_WRAPPER( [ ]( std::string s )
		{
			EXPECT_EQ( s, "world" );
		} ) )
	);
}
