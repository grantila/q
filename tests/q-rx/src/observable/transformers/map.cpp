
#include "../../q-rx-test.hpp"

Q_TEST_MAKE_SCOPE( ob_trans_map );

TEST_F( ob_trans_map, sync_int_to_int )
{
	std::vector< int > vec_input{ 1, 2, 3 };

	auto o_in = q::rx::with( queue, vec_input );

	int counter = 0;
	std::vector< int > vec_expected{ 2, 4, 6 };

	auto consumer = [ this, &counter, &vec_expected ]( int i )
	{
		EXPECT_EQ( vec_expected[ counter++ ], i );
	};

/*
	EXPECT_OBSERVABLE_EQ(
		o_in,
		q::rx::with( queue, std::vector< int >{ 4, 8, 12 } )
	);

	EXPECT_OBSERVABLE_EQ( ( std::vector< int >{ 4, 8, 12 } ), o_in );
*/

	run(
		o_in
		.map( q::rx::f::mul( 2 ) )
		.consume( EXPECT_N_CALLS_WRAPPER( 3, consumer ) )
	);
}
