
#include "../q-rx-test.hpp"

#include <q-rx/functional.hpp>

Q_TEST_MAKE_SCOPE( observable_creation );

TEST_F( observable_creation, empty )
{
	auto o_empty = q::rx::observable< int >::empty( queue );

	run( o_empty.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ) );
}

TEST_F( observable_creation, error )
{
	auto o_error = q::rx::observable< int >::error( queue, Error( ) );

	EVENTUALLY_EXPECT_REJECTION_WITH(
		o_error.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ),
		Error
	);
}

TEST_F( observable_creation, with_vector )
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

TEST_F( observable_creation, with_channel )
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

Q_TEST_MAKE_SCOPE( ob_creat_create );

TEST_F( ob_creat_create, with_data_success )
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

TEST_F( ob_creat_create, with_data_failed )
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
