
#include "../q-rx-test.hpp"

#include <q-rx/functional.hpp>

Q_TEST_MAKE_SCOPE( observable_consumption );

TEST_F( observable_consumption, DISABLED_sync_void )
{
/*
	auto o_empty = q::rx::observable< int >::empty( queue );

	run( o_empty.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ) );
*/
}

TEST_F( observable_consumption, DISABLED_sync_int )
{
/*
	std::vector< int > vec_int{ 1, 2, 3 };
	auto o_from_vector = q::rx::with( vec_int, queue );

	int counter = 1;

	auto consumer = EXPECT_N_CALLS_WRAPPER( 3, [ &counter ]( int value )
	{
		EXPECT_EQ( value, counter );
		++counter;
	} );

	run( o_from_vector.consume( consumer ) );
*/
}

TEST_F( observable_consumption, DISABLED_sync_int_string )
{
}

TEST_F( observable_consumption, DISABLED_sync_promise_of_void )
{
}

TEST_F( observable_consumption, DISABLED_sync_promise_of_int )
{
}

TEST_F( observable_consumption, DISABLED_sync_promise_of_int_string )
{
}

TEST_F( observable_consumption, DISABLED_sync_promise_of_void_throw_sync )
{
}

TEST_F( observable_consumption, DISABLED_sync_promise_of_int_throw_sync )
{
}

TEST_F( observable_consumption, DISABLED_sync_promise_of_int_string_throw_sync )
{
}

TEST_F( observable_consumption, DISABLED_sync_promise_of_void_throw_async )
{
}

TEST_F( observable_consumption, DISABLED_sync_promise_of_int_throw_async )
{
}

TEST_F( observable_consumption, DISABLED_sync_promise_of_int_string_throw_async )
{
}

TEST_F( observable_consumption, DISABLED_async_void )
{
/*
	auto o_empty = q::rx::observable< int >::empty( queue );

	run( o_empty.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ) );
*/
}

TEST_F( observable_consumption, DISABLED_async_int )
{
/*
	std::vector< int > vec_int{ 1, 2, 3 };
	auto o_from_vector = q::rx::with( vec_int, queue );

	int counter = 1;

	auto consumer = EXPECT_N_CALLS_WRAPPER( 3, [ &counter ]( int value )
	{
		EXPECT_EQ( value, counter );
		++counter;
	} );

	run( o_from_vector.consume( consumer ) );
*/
}

TEST_F( observable_consumption, DISABLED_async_int_string )
{
}

TEST_F( observable_consumption, DISABLED_async_promise_of_void )
{
}

TEST_F( observable_consumption, DISABLED_async_promise_of_int )
{
}

TEST_F( observable_consumption, DISABLED_async_promise_of_int_string )
{
}

TEST_F( observable_consumption, DISABLED_async_promise_of_void_throw_sync )
{
}

TEST_F( observable_consumption, DISABLED_async_promise_of_int_throw_sync )
{
}

TEST_F( observable_consumption, DISABLED_async_promise_of_int_string_throw_sync )
{
}

TEST_F( observable_consumption, DISABLED_async_promise_of_void_throw_async )
{
}

TEST_F( observable_consumption, DISABLED_async_promise_of_int_throw_async )
{
}

TEST_F( observable_consumption, DISABLED_async_promise_of_int_string_throw_async )
{
}
