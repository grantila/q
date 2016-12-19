
#include "../../q-rx-test.hpp"

Q_TEST_MAKE_SCOPE( ob_create_timer );

TEST_F( ob_create_timer, vector )
{
	auto dur = std::chrono::milliseconds( 1 );
	auto o_timer = q::rx::observable< int >::timer( dur, 13, { queue } );

	int counter = 1;

	auto consumer = EXPECT_N_CALLS_WRAPPER( 20, [ &counter ]( int value )
	{
		EXPECT_EQ( value, 13 );

		if ( counter == 20 )
			Q_THROW( q::channel_closed_exception( ) );

		++counter;
	} );

	run( o_timer.consume( consumer ) );
}
