
#include "helpers.hpp"

Q_TEST_MAKE_SCOPE( Map );

class set_default {};

TEST_F( Map, MapSyncFilterSync )
{
	std::vector< int > vec{ 1, 2, 3, 4, 5 };

	auto promise = q::with( queue, vec )
	.map_sync( [ ]( int i ) -> float
	{
		return (float)i * 3.141592;
	} )
	.then( [ ]( std::vector< float >&& result )
	{
		EXPECT_FLOAT_EQ( 3.141592,  result[ 0 ] );
		EXPECT_FLOAT_EQ( 6.2831841, result[ 1 ] );
		EXPECT_FLOAT_EQ( 9.42478,   result[ 2 ] );
		EXPECT_FLOAT_EQ( 12.566368, result[ 3 ] );
		EXPECT_FLOAT_EQ( 15.70796,  result[ 4 ] );
		EXPECT_EQ( 5, result.size( ) );
		return std::move( result );
	} )
	.map_sync( [ ]( float f ) -> int
	{
		return ( int )f;
	} )
	.filter_sync( [ ]( int i ) -> bool
	{
		return i % 2 == 0;
	} )
	.then( [ ]( std::vector< int >&& result )
	{
		EXPECT_EQ( 6,  result[ 0 ] );
		EXPECT_EQ( 12, result[ 1 ] );
		EXPECT_EQ( 2,  result.size( ) );
	} );

	run( promise );
}

TEST_F( Map, ReduceSync )
{
	std::vector< int > vec{ 1, 2, 3, 4, 5 };

	auto promise = q::with( queue, vec )
	.reduce_sync( [ &vec ]( float prev, int cur, size_t index ) -> float
	{
		return prev + ((float)cur) / 2;
	}, 0.0 )
	.then( [ ]( float result )
	{
		EXPECT_FLOAT_EQ( 7.5f, result );
	} );

	run( promise );
}
