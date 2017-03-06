
#include <q-test/q-test.hpp>

Q_TEST_MAKE_SCOPE( forward );

TEST_F( forward, forward_empty )
{
	run(
		q::with( queue )
		.forward( )
		.then( EXPECT_CALL_WRAPPER( [ ]( ) { } ) )
	);
}

TEST_F( forward, forward_one_value )
{
	run(
		q::with( queue )
		.forward( 5 )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( int i )
		{
			EXPECT_EQ( 5, i );
		} ) )
	);
}

TEST_F( forward, forward_two_values )
{
	run(
		q::with( queue )
		.forward( 5, std::string( "foo" ) )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( int i, std::string s )
		{
			EXPECT_EQ( 5, i );
			EXPECT_EQ( "foo", s );
		} ) )
	);
}

TEST_F( forward, forward_with_exception )
{
	run(
		q::reject< >( queue, Error( ) )
		.forward( 5 )
		.then( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) )
		.fail( EXPECT_CALL_WRAPPER( [ ]( const Error& ) { } ) )
	);
}
