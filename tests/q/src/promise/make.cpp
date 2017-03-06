
#include <q-test/q-test.hpp>

Q_TEST_MAKE_SCOPE( make );

TEST_F( make, by_expression_with_value )
{
	auto promise = q::make_promise( queue,
		[ ]( )
		{
			return 17;
		}
	)
	.then( EXPECT_CALL_WRAPPER( [ ]( int value )
	{
		EXPECT_EQ( value, 17 );
	} ) );

	run( std::move( promise ) );
}

TEST_F( make, by_expression_with_exception )
{
	auto promise = q::make_promise( queue,
		[ ]( ) -> int
		{
			Q_THROW( Error( ) );
		}
	)
	.then( EXPECT_NO_CALL( void, int )( ) )
	.fail( EXPECT_CALL( void, Error& )( ) )
	.fail( EXPECT_NO_CALL( void, std::exception_ptr )( ) );

	run( std::move( promise ) );
}

TEST_F( make, async_with_value )
{
	auto promise = q::make_promise( queue,
		[ ]( q::resolver< int, int > resolve,
			q::rejecter< int, int > )
		{
			resolve( 1, 2 );
		}
	)
	.then( EXPECT_CALL_WRAPPER( [ ]( int a, int b )
	{
		EXPECT_EQ( a, 1 );
		EXPECT_EQ( b, 2 );
	} ) );

	run( std::move( promise ) );
}


TEST_F( make, async_with_exception )
{
	auto promise = q::make_promise( queue,
		[ ]( q::resolver< int, int >,
			q::rejecter< int, int > reject )
		{
			reject( Error( ) );
		}
	)
	.then( EXPECT_NO_CALL( void, int, int )( ) )
	.fail( EXPECT_CALL( void, Error& )( ) )
	.fail( EXPECT_NO_CALL( void, std::exception_ptr )( ) );

	run( std::move( promise ) );
}

#ifdef LIBQ_WITH_CPP14

TEST_F( make, by_lambda_auto )
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

	run( std::move( promise3 ) );
}

#endif
