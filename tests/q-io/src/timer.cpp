
#include "q-io-test.hpp"

#include <q-test/expect.hpp>

QIO_TEST_MAKE_SCOPE( timer );

TEST_F( timer, on_void )
{
	auto promise = q::with( io_queue )
	.delay( std::chrono::milliseconds( 1 ) )
	.then( EXPECT_CALL_WRAPPER( [ ]( ) { } ) );

	run( std::move( promise ) );
}

TEST_F( timer, on_value )
{
	auto promise = q::with( io_queue, 4711 )
	.delay( std::chrono::milliseconds( 1 ) )
	.then( EXPECT_CALL_WRAPPER(
		[ ]( int i )
		{
			EXPECT_EQ( i, 4711 );
		}
	) );

	run( std::move( promise ) );
}
