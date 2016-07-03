
#include <q-test/q-test.hpp>
#include <q-test/expect.hpp>

Q_TEST_MAKE_SCOPE( promisify );

TEST_F( promisify, function_returning_void )
{
	auto fn = EXPECT_CALL_WRAPPER( [ ]( ) { } );

	auto promise_fn = q::promisify( queue, fn );

	EVENTUALLY_EXPECT_RESOLUTION( promise_fn( ) );
}

TEST_F( promisify, function_returning_int )
{
	auto fn = EXPECT_N_CALLS_WRAPPER( 2, [ ]( ) { return 5; } );

	auto promise_fn = q::promisify( queue, fn );

	EVENTUALLY_EXPECT_RESOLUTION( promise_fn( ) );
	EVENTUALLY_EXPECT_EQ( promise_fn( ), 5 );
}

TEST_F( promisify, function_getting_int_returning_void )
{
	auto promisified = q::promisify( queue, [ ]( int i )
	{
		EXPECT_EQ( i, 5 );
	} );

	run(
		promisified( 5 )
		.then( EXPECT_CALL_WRAPPER( [ ]( ) { } ) )
	);
}

TEST_F( promisify, function_getting_int_returning_int )
{
	auto promisified = q::promisify( queue, [ ]( int i ) -> int
	{
		return i * 2;
	} );

	EVENTUALLY_EXPECT_EQ( promisified( 5 ), 10 );
}

TEST_F( promisify, function_returning_empty_promise )
{
	auto queue = this->queue;

	auto fn = EXPECT_CALL_WRAPPER(
		[ queue ]( ) { return q::with( queue ); }
	);

	auto promise_fn = q::promisify( queue, fn );

	EVENTUALLY_EXPECT_RESOLUTION( promise_fn( ) );
}

TEST_F( promisify, function_returning_int_promise )
{
	auto queue = this->queue;

	auto fn = EXPECT_N_CALLS_WRAPPER( 2,
		[ queue ]( ) { return q::with( queue, 5 ); }
	);

	auto promise_fn = q::promisify( queue, fn );

	EVENTUALLY_EXPECT_RESOLUTION( promise_fn( ) );
	EVENTUALLY_EXPECT_EQ( promise_fn( ), 5 );
}
