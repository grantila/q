
#include <q-test/q-test.hpp>

Q_TEST_MAKE_SCOPE( promisify );

TEST_F( promisify, get_void_return_void )
{
	auto promisified = q::promisify( queue, [ ]( )
	{
		EXPECT_TRUE( true );
	} );

	auto promise = promisified( )
	.then( EXPECT_CALL_WRAPPER( [ ]( )
	{
		EXPECT_TRUE( true );
	} ) );

	run( std::move( promise ) );
}

TEST_F( promisify, get_void_return_int )
{
	auto promisified = q::promisify( queue, [ ]( )
	{
		return 5;
	} );

	auto promise = promisified( )
	.then( EXPECT_CALL_WRAPPER( [ ]( int value )
	{
		EXPECT_EQ( value, 5 );
	} ) );

	run( std::move( promise ) );
}

TEST_F( promisify, get_int_return_void )
{
	auto promisified = q::promisify( queue, [ ]( int i )
	{
		EXPECT_EQ( i, 5 );
	} );

	auto promise = promisified( 5 )
	.then( EXPECT_CALL_WRAPPER( [ ]( )
	{
		EXPECT_TRUE( true );
	} ) );

	run( std::move( promise ) );
}

TEST_F( promisify, get_int_return_int )
{
	auto promisified = q::promisify( queue, [ ]( int i ) -> int
	{
		return i * 2;
	} );

	auto promise = promisified( 5 )
	.then( EXPECT_CALL_WRAPPER( [ ]( int value )
	{
		EXPECT_EQ( value, 10 );
	} ) );

	run( std::move( promise ) );
}
