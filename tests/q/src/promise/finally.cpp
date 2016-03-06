
#include <q-test/q-test.hpp>

Q_TEST_MAKE_SCOPE( Finally );

TEST_F( Finally, SynchronousWithValue )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( ) -> long
		{
			return 17;
		} ) )
		.finally( EXPECT_CALL_WRAPPER( spy )(
		[ ]( )
		{
			;
		} ) )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( long value )
		{
			EXPECT_EQ( 17, value );
		} ) )
	);
}

TEST_F( Finally, SynchronousWithException )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( ) -> long
		{
			throw Error( );
		} ) )
		.finally( EXPECT_CALL_WRAPPER( spy )(
		[ ]( )
		{
			;
		} ) )
		.fail( EXPECT_CALL_WRAPPER( spy )(
		[ ]( Error& e ) -> long
		{
			return 0;
		} ) )
	);
}

TEST_F( Finally, SynchronousWithValueFailedFinally )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( ) -> long
		{
			return 17;
		} ) )
		.finally( EXPECT_CALL_WRAPPER( spy )(
		[ ]( )
		{
			throw Error( );
		} ) )
		.then( EXPECT_NO_CALL_WRAPPER( spy )(
		[ ]( long value )
		{
		} ) )
		.fail( EXPECT_CALL_WRAPPER( spy )(
		[ ]( Error& e )
		{
		} ) )
	);
}

TEST_F( Finally, AsynchronousWithValue )
{
	auto queue = this->queue;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( ) -> long
		{
			return 17;
		} ) )
		.finally( EXPECT_CALL_WRAPPER( spy )(
		[ queue ]( )
		{
			return q::with( queue );
		} ) )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( long value )
		{
			EXPECT_EQ( 17, value );
		} ) )
	);
}

TEST_F( Finally, AsynchronousWithException )
{
	auto queue = this->queue;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( ) -> long
		{
			throw Error( );
		} ) )
		.finally( EXPECT_CALL_WRAPPER( spy )(
		[ queue ]( )
		{
			return q::with( queue );
		} ) )
		.then( EXPECT_NO_CALL_WRAPPER( spy )(
		[ ]( long value )
		{
		} ) )
		.fail( EXPECT_CALL_WRAPPER( spy )(
		[ ]( Error& )
		{
		} ) )
	);
}

TEST_F( Finally, AsynchronousWithValueSynchronousFailedFinally )
{
	auto queue = this->queue;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( ) -> long
		{
			return 17;
		} ) )
		.finally( EXPECT_CALL_WRAPPER( spy )(
		[ queue ]( )
		-> q::promise< std::tuple< > >
		{
			throw Error( );
		} ) )
		.then( EXPECT_NO_CALL_WRAPPER( spy )(
		[ ]( long value )
		{
		} ) )
		.fail( EXPECT_CALL_WRAPPER( spy )(
		[ ]( Error& e )
		{
		} ) )
	);
}

TEST_F( Finally, AsynchronousWithValueAsynchronousFailedFinally )
{
	auto queue = this->queue;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( ) -> long
		{
			return 17;
		} ) )
		.finally( EXPECT_CALL_WRAPPER( spy )(
		[ queue ]( )
		{
			return q::with( queue )
			.then( [ ]( )
			{
				throw Error( );
			} );
		} ) )
		.then( EXPECT_NO_CALL_WRAPPER( spy )(
		[ ]( long value )
		{
		} ) )
		.fail( EXPECT_CALL_WRAPPER( spy )(
		[ ]( Error& e )
		{
		} ) )
	);
}
