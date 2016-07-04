
#include "../../q-rx-test.hpp"

#include <q-rx/functional.hpp>

Q_TEST_MAKE_SCOPE( ob_trans_map );

TEST_F( ob_trans_map, sync_and_async_int_to_int )
{
	std::vector< int > vec_input{ 1, 2, 3 };

	auto o_in = q::rx::with( queue, vec_input );

	int counter = 4;

	auto consumer = [ this, &counter ]( int i )
	{
		EXPECT_EQ( counter, i );
		counter += 4;
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
/* */
		.map( [ this ]( int i )
		{
			return i * 2;// q::with( queue, i * 2 );
		} )
/* */
		.consume( EXPECT_N_CALLS_WRAPPER( 3, consumer ) )
	);
}
