
#include "../../q-rx-test.hpp"

Q_TEST_MAKE_SCOPE( ob_trans_buffer );

TEST_F( ob_trans_buffer, count_zero )
{
	auto o_in = q::rx::observable< int >::range( 1, 5, { queue } );

	EXPECT_THROW( o_in.buffer( 0 ), q::length_error );
}

TEST_F( ob_trans_buffer, count_one )
{
	auto o_in = q::rx::observable< int >::range( 1, 5, { queue } );

	int counter = 0;
	std::vector< std::vector< int > > values{
		{ 1 }, { 2 }, { 3 }, { 4 }, { 5 }
	};

	auto consumer = [ &counter, &values ]( std::vector< int >&& v )
	{
		EXPECT_EQ( values[ counter ], v );
		++counter;
	};

	run(
		o_in
		.buffer( 1 )
		.consume( EXPECT_N_CALLS_WRAPPER( 5, consumer ) )
	);
}

TEST_F( ob_trans_buffer, count_two )
{
	auto o_in = q::rx::observable< int >::range( 1, 5, { queue } );

	int counter = 0;
	std::vector< std::vector< int > > values{
		{ 1, 2 },
		{ 3, 4 },
		{ 5 }
	};

	auto consumer = [ &counter, &values ]( std::vector< int >&& v )
	{
		EXPECT_EQ( values[ counter ], v );
		++counter;
	};

	run(
		o_in
		.buffer( 2 )
		.consume( EXPECT_N_CALLS_WRAPPER( 3, consumer ) )
	);
}

TEST_F( ob_trans_buffer, count_same_size )
{
	auto o_in = q::rx::observable< int >::range( 1, 5, { queue } );

	int counter = 0;
	std::vector< std::vector< int > > values{
		{ 1, 2, 3, 4, 5 }
	};

	auto consumer = [ &counter, &values ]( std::vector< int >&& v )
	{
		EXPECT_EQ( values[ counter ], v );
		++counter;
	};

	run(
		o_in
		.buffer( 5 )
		.consume( EXPECT_N_CALLS_WRAPPER( 1, consumer ) )
	);
}

TEST_F( ob_trans_buffer, count_larger )
{
	auto o_in = q::rx::observable< int >::range( 1, 5, { queue } );

	int counter = 0;
	std::vector< std::vector< int > > values{
		{ 1, 2, 3, 4, 5 }
	};

	auto consumer = [ &counter, &values ]( std::vector< int >&& v )
	{
		EXPECT_EQ( values[ counter ], v );
		++counter;
	};

	run(
		o_in
		.buffer( 7 )
		.consume( EXPECT_N_CALLS_WRAPPER( 1, consumer ) )
	);
}

TEST_F( ob_trans_buffer, count_empty_zero )
{
	auto o_in = q::rx::observable< int >::empty( { queue } );

	EXPECT_THROW( o_in.buffer( 0 ), q::length_error );
}

TEST_F( ob_trans_buffer, count_empty_one )
{
	auto o_in = q::rx::observable< int >::empty( { queue } );

	auto consumer = [  ]( std::vector< int >&& v ) { };

	run(
		o_in
		.buffer( 1 )
		.consume( EXPECT_NO_CALL_WRAPPER( consumer ) )
	);
}

TEST_F( ob_trans_buffer, count_empty_two )
{
	auto o_in = q::rx::observable< int >::empty( { queue } );

	auto consumer = [  ]( std::vector< int >&& v ) { };

	run(
		o_in
		.buffer( 2 )
		.consume( EXPECT_NO_CALL_WRAPPER( consumer ) )
	);
}

// TODO: ... same as above but exception is thrown
