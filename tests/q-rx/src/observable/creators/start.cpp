
#include "../../q-rx-test.hpp"

Q_TEST_MAKE_SCOPE( ob_create_start );

TEST_F( ob_create_start, void_return_success )
{
	auto starter = [ ]( ) { };

	auto o_start = q::rx::observable< void >::start( starter, { queue } );

	run( o_start.consume( EXPECT_CALL_WRAPPER( [ ]( ) { } ) ) );
}

TEST_F( ob_create_start, void_return_failure )
{
	auto starter = [ ]( )
	{
		Q_THROW( Error( ) );
	};

	auto o_start = q::rx::observable< void >::start( starter, { queue } );

	EVENTUALLY_EXPECT_REJECTION_WITH(
		o_start.consume( EXPECT_NO_CALL_WRAPPER( [ ]( ) { } ) ),
		Error
	);

	run( q::with( queue ) );
}

TEST_F( ob_create_start, async_void_return_success )
{
	auto starter = [ this ]( )
	{
		return q::with( queue );
	};

	auto o_start = q::rx::observable< void >::start( starter, { queue } );

	run( o_start.consume( EXPECT_CALL_WRAPPER( [ ]( ) { } ) ) );
}

TEST_F( ob_create_start, async_void_return_failure )
{
	auto starter = [ this ]( )
	{
		return q::reject< q::arguments< > >( queue, Error( ) );
	};

	auto o_start = q::rx::observable< void >::start( starter, { queue } );

	EVENTUALLY_EXPECT_REJECTION_WITH(
		o_start.consume( EXPECT_NO_CALL_WRAPPER( [ ]( ) { } ) ),
		Error
	);

	run( q::with( queue ) );
}

TEST_F( ob_create_start, async_void_return_failure_sync )
{
	auto starter = [ ]( )
	-> q::promise< std::tuple< > >
	{
		Q_THROW( Error( ) );
	};

	auto o_start = q::rx::observable< void >::start( starter, { queue } );

	EVENTUALLY_EXPECT_REJECTION_WITH(
		o_start.consume( EXPECT_NO_CALL_WRAPPER( [ ]( ) { } ) ),
		Error
	);

	run( q::with( queue ) );
}

TEST_F( ob_create_start, int_return_success )
{
	auto starter = [ ]( )
	{
		return 5;
	};

	auto o_start = q::rx::observable< int >::start( starter, { queue } );

	auto consumer = [ ]( int value )
	{
		EXPECT_EQ( 5, value );
	};

	run( o_start.consume( EXPECT_CALL_WRAPPER( consumer ) ) );
}

TEST_F( ob_create_start, int_return_failure )
{
	auto starter = [ ]( )
	-> int
	{
		Q_THROW( Error( ) );
	};

	auto o_start = q::rx::observable< int >::start( starter, { queue } );

	EVENTUALLY_EXPECT_REJECTION_WITH(
		o_start.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ),
		Error
	);

	run( q::with( queue ) );
}

TEST_F( ob_create_start, async_int_return_success )
{
	auto starter = [ this ]( )
	{
		return q::with( queue, 5 );
	};

	auto o_start = q::rx::observable< int >::start( starter, { queue } );

	auto consumer = [ ]( int value )
	{
		EXPECT_EQ( 5, value );
	};

	run( o_start.consume( EXPECT_CALL_WRAPPER( consumer ) ) );
}

TEST_F( ob_create_start, async_int_return_failure )
{
	auto starter = [ this ]( )
	{
		return q::reject< q::arguments< int > >( queue, Error( ) );
	};

	auto o_start = q::rx::observable< int >::start( starter, { queue } );

	EVENTUALLY_EXPECT_REJECTION_WITH(
		o_start.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ),
		Error
	);

	run( q::with( queue ) );
}

TEST_F( ob_create_start, async_int_return_failure_sync )
{
	auto starter = [ ]( )
	-> q::promise< std::tuple< int > >
	{
		Q_THROW( Error( ) );
	};

	auto o_start = q::rx::observable< int >::start( starter, { queue } );

	EVENTUALLY_EXPECT_REJECTION_WITH(
		o_start.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ),
		Error
	);

	run( q::with( queue ) );
}
