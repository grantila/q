
#include <q-test/q-test.hpp>

Q_TEST_MAKE_SCOPE( delay );

TEST_F( delay, with_value )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> long
		{
			return 17;
		} ) )
		.delay( std::chrono::nanoseconds( 1 ) )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( long value )
		{
			EXPECT_EQ( 17, value );
		} ) )
	);
}

TEST_F( delay, with_exception )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( )
		{
			Q_THROW( Error( ) );
		} ) )
		.delay( std::chrono::nanoseconds( 1 ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( Error& e ) { } ) )
		.then( [ ]( ) { } )
	);
}
