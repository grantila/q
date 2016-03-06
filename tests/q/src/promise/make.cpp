
#include <q-test/q-test.hpp>

Q_TEST_MAKE_SCOPE( Make );

TEST_F( Make, ByExpressionWithValue )
{
	auto promise = q::make_promise( queue,
		[ ]( )
		{
			return 17;
		}
	)
	.then( EXPECT_CALL_WRAPPER( spy )( [ ]( int value )
	{
		EXPECT_EQ( value, 17 );
	} ) );

	run( promise );
}

TEST_F( Make, ByExpressionWithException )
{
	auto promise = q::make_promise( queue,
		[ ]( ) -> int
		{
			throw Error( );
		}
	)
	.then( EXPECT_NO_CALL( spy, void, int )( ) )
	.fail( EXPECT_CALL( spy, void, Error& )( ) )
	.fail( EXPECT_NO_CALL( spy, void, std::exception_ptr )( ) );

	run( promise );
}

TEST_F( Make, AsyncWithValue )
{
	auto promise = q::make_promise( queue,
		[ ]( q::resolver< int, int > resolve,
			q::rejecter< int, int > reject )
		{
			resolve( 1, 2 );
		}
	)
	.then( EXPECT_CALL_WRAPPER( spy )( [ ]( int a, int b )
	{
		EXPECT_EQ( a, 1 );
		EXPECT_EQ( b, 2 );
	} ) );

	run( promise );
}


TEST_F( Make, AsyncWithException )
{
	auto promise = q::make_promise( queue,
		[ ]( q::resolver< int, int > resolve,
			q::rejecter< int, int > reject )
		{
			reject( Error( ) );
		}
	)
	.then( EXPECT_NO_CALL( spy, void, int, int )( ) )
	.fail( EXPECT_CALL( spy, void, Error& )( ) )
	.fail( EXPECT_NO_CALL( spy, void, std::exception_ptr )( ) );

	run( promise );
}

#ifdef LIBQ_WITH_CPP14

TEST_F( Make, ByLambdaAuto )
{
	auto promise1 = q::make_promise( queue,
		[ ]( q::resolver< int > resolve, q::rejecter< int > reject )
		{
			resolve( 4711 );
		}
	);

	auto promise2 = q::make_promise_of< int >( queue,
		[ ]( auto resolve, auto reject )
		{
			resolve( 4712 );
		}
	);

	auto promise3 = q::all( std::move( promise1 ), std::move( promise2 ) )
	.then( [ ]( int a, int b )
	{
		EXPECT_EQ( a, 4711 );
		EXPECT_EQ( b, 4712 );
	} );

	run( promise3 );
}

#endif
