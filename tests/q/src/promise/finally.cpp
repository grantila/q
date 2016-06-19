
#include <q-test/q-test.hpp>

Q_TEST_MAKE_SCOPE( finally );

TEST_F( finally, synchronous_with_value )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> long
		{
			return 17;
		} ) )
		.finally( EXPECT_CALL_WRAPPER(
		[ ]( )
		{
			;
		} ) )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( long value )
		{
			EXPECT_EQ( 17, value );
		} ) )
	);
}

TEST_F( finally, synchronous_with_exception )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> long
		{
			throw Error( );
		} ) )
		.finally( EXPECT_CALL_WRAPPER(
		[ ]( )
		{
			;
		} ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( Error& e ) -> long
		{
			return 0;
		} ) )
		.then( [ ]( long ) { } )
	);
}

TEST_F( finally, synchronous_with_value_failed_finally )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> long
		{
			return 17;
		} ) )
		.finally( EXPECT_CALL_WRAPPER(
		[ ]( )
		{
			throw Error( );
		} ) )
		.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( long value )
		{
		} ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( Error& e )
		{
		} ) )
	);
}

TEST_F( finally, asynchronous_with_value )
{
	auto queue = this->queue;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> long
		{
			return 17;
		} ) )
		.finally( EXPECT_CALL_WRAPPER(
		[ queue ]( )
		{
			return q::with( queue );
		} ) )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( long value )
		{
			EXPECT_EQ( 17, value );
		} ) )
	);
}

TEST_F( finally, asynchronous_with_exception )
{
	auto queue = this->queue;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> long
		{
			throw Error( );
		} ) )
		.finally( EXPECT_CALL_WRAPPER(
		[ queue ]( )
		{
			return q::with( queue );
		} ) )
		.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( long value )
		{
		} ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( Error& )
		{
		} ) )
	);
}

TEST_F( finally, asynchronous_with_value_synchronous_failed_finally )
{
	auto queue = this->queue;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> long
		{
			return 17;
		} ) )
		.finally( EXPECT_CALL_WRAPPER(
		[ queue ]( )
		-> q::promise< std::tuple< > >
		{
			throw Error( );
		} ) )
		.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( long value )
		{
		} ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( Error& e )
		{
		} ) )
	);
}

TEST_F( finally, asynchronous_with_value_asynchronous_failed_finally )
{
	auto queue = this->queue;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> long
		{
			return 17;
		} ) )
		.finally( EXPECT_CALL_WRAPPER(
		[ queue ]( )
		{
			return q::with( queue )
			.then( [ ]( )
			{
				throw Error( );
			} );
		} ) )
		.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( long value )
		{
		} ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( Error& e )
		{
		} ) )
	);
}
