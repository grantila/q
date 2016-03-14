
#include "q-io-test.hpp"

QIO_TEST_MAKE_SCOPE( timer );

TEST_F( timer, on_void )
{
	auto promise = q::with( queue )
	.then( io_dispatcher->delay( std::chrono::milliseconds( 1 ) ) )
	.then( EXPECT_CALL_WRAPPER( spy )( [ ]( ) { } ) );

	run( std::move( promise ) );
}

TEST_F( timer, on_value )
{
	auto promise = q::with( queue, 4711 )
	.then( io_dispatcher->delay( std::chrono::milliseconds( 1 ) ) )
	.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( int i )
		{
			EXPECT_EQ( i, 4711 );
		}
	) );

	run( std::move( promise ) );
}
