
#include "../q-rx-test.hpp"

#include <q-rx/functional.hpp>

Q_TEST_MAKE_SCOPE( observable_creation );

TEST_F( observable_creation, empty )
{
	auto o_empty = q::rx::observable< int >::empty( queue );

	run( o_empty.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ) );
}

TEST_F( observable_creation, with_vector )
{
	std::vector< int > vec_int{ 1, 2, 3 };
	auto o_from_vector = q::rx::with( vec_int, queue );

	int counter = 1;

	auto consumer = EXPECT_N_CALLS_WRAPPER( 3, [ &counter ]( int value )
	{
		EXPECT_EQ( value, counter );
		++counter;
	} );

	run( o_from_vector.consume( consumer ) );
}
