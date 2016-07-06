
#include "../../q-rx-test.hpp"

Q_TEST_MAKE_SCOPE( ob_create_with );

TEST_F( ob_create_with, vector )
{
	std::vector< int > vec_int{ 1, 2, 3 };
	auto o_from_vector = q::rx::with( queue, vec_int );

	int counter = 1;

	auto consumer = EXPECT_N_CALLS_WRAPPER( 3, [ &counter ]( int value )
	{
		EXPECT_EQ( value, counter );
		++counter;
	} );

	run( o_from_vector.consume( consumer ) );
}

TEST_F( ob_create_with, DISABLED_channel )
{
}
