
#include "../../q-rx-test.hpp"

Q_TEST_MAKE_SCOPE( ob_create_create );

TEST_F( ob_create_create, with_data_success )
{
	auto o_create = q::rx::observable< int >::create(
		[ ]( auto observer )
		{
			observer.onNext( 1 );
			observer.onNext( 2 );
			observer.onNext( 3 );
			observer.onCompleted( );
		},
		{ queue }
	);

	int counter = 1;

	auto consumer = EXPECT_N_CALLS_WRAPPER( 3, [ &counter ]( int value )
	{
		EXPECT_EQ( value, counter );
		++counter;
	} );

	run( o_create.consume( consumer ) );
}

TEST_F( ob_create_create, with_data_failed )
{
	auto o_create = q::rx::observable< int >::create(
		[ ]( auto observer )
		{
			observer.onNext( 1 );
			observer.onNext( 2 );
			observer.onNext( 3 );
			observer.onError( Error( ) );
		},
		{ queue }
	);

	int counter = 1;

	auto consumer = EXPECT_N_CALLS_WRAPPER( 3, [ &counter ]( int value )
	{
		EXPECT_EQ( value, counter );
		++counter;
	} );

	EVENTUALLY_EXPECT_REJECTION_WITH( o_create.consume( consumer ), Error );

	run( q::with( queue ) );
}
