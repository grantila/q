
#include "../core.hpp"

Q_TEST_MAKE_SCOPE( expect_spy );

TEST_F( expect_spy, expect_call_generated )
{
	int expected = 0;

	auto p = q::with( queue, 6 )
	.then( EXPECT_CALL( int, int )( 3 ) )
	.then( [ & ]( int i ) { expected = i; } )
	.then( [ & ]( ) { EXPECT_EQ( expected, 3 ); } );

	run( std::move( p ) );
}

TEST_F( expect_spy, expect_no_call_generated )
{
	auto p = q::with( queue, 6 )
	.fail( EXPECT_NO_CALL( int, std::exception_ptr )( 3 ) )
	.then( [ ]( int ) { } );

	run( std::move( p ) );
}

TEST_F( expect_spy, expect_n_call_generated )
{
	// Expect this callback to be called exactly 3 times
	auto callback = EXPECT_N_CALLS( 3, int, int )( 0 );

	auto p = q::with( queue, 6 )
	.then( callback )
	.then( callback )
	.then( callback )
	.then( [ ]( int ) { } );

	run( std::move( p ) );
}

TEST_F( expect_spy, expect_call_wrapper )
{
	auto p = q::with( queue, 6 )
	.then( EXPECT_CALL_WRAPPER( [ ]( int i ) { return i; } ) )
	.then( [ ]( int i ) { EXPECT_EQ( i, 6 ); } );

	run( std::move( p ) );
}

TEST_F( expect_spy, expect_no_call_wrapper )
{
	auto p = q::with( queue, 6 )
	.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr ) { return 3; }
	) )
	.then( [ ]( int i ) { EXPECT_EQ( i, 6 ); } );

	run( std::move( p ) );
}

TEST_F( expect_spy, expect_n_call_wrapper )
{
	// Expect this callback to be called exactly 3 times
	auto callback = EXPECT_N_CALLS_WRAPPER( 3,
		[ ]( int i ) { return i + 1; }
	);

	auto p = q::with( queue, 6 )
	.then( callback )
	.then( callback )
	.then( callback )
	.then( [ ]( int i ) { EXPECT_EQ( i, 9 ); } );

	run( std::move( p ) );
}
