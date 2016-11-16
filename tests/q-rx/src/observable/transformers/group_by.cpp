
#include "../../q-rx-test.hpp"

#include <map>
#include <unordered_map>

Q_TEST_MAKE_SCOPE( ob_trans_group_by );

namespace {

auto on_error = [ ]( std::exception_ptr err ) { };

} // anonymous namespace

struct hashable
{
	int i;

	bool operator==( const hashable& other ) const
	{
		return other.i == i;
	}
};

namespace std {

template<> struct hash< hashable >
{
	typedef hashable argument_type;
	typedef std::size_t result_type;

	result_type operator( )( const hashable& h ) const
	{
		return std::hash< int >{ }( h.i );
	}
};

} // namespace std

struct comparable
{
	int i;

	bool operator<( const comparable& other ) const
	{
		return i < other.i;
	}

	bool operator==( const comparable& other ) const
	{
		return other.i == i;
	}
};

TEST_F( ob_trans_group_by, empty_observable_hash )
{
	auto o_in = q::rx::observable< int >::empty( { queue } );

	auto consumer = [ ]( int key, q::rx::observable< int > o ) { };

	run(
		o_in
		.group_by( [ ]( int i ) -> int { return !( i % 2 ); } )
		.consume( EXPECT_NO_CALL_WRAPPER( consumer ) )
	);
}

TEST_F( ob_trans_group_by, sync_int_hash )
{
	auto o_in = q::rx::observable< int >::range( 1, 10, { queue } );

	std::vector< int > expected_keys{ 1, 0 };
	std::size_t key_counter = 0;

	std::vector< std::vector< int > > expected_values{
		{ 2, 4, 6, 8, 10 }, // index 0 = n % 2
		{ 1, 3, 5, 7, 9 }  // index 1 = n % 2
	};
	std::size_t value_counter[ ] = { 0, 0 };

	auto grouper = [ ]( int i ) -> int { return i % 2; };

	auto consumer = [ &, this ]( int key, q::rx::observable< int > o )
	{
		EXPECT_EQ( expected_keys[ key_counter++ ], key );

		auto consumer = [ &, key ]( int val )
		{
			EXPECT_EQ(
				expected_values[ key ][ value_counter[ key ]++ ],
				val
			);
		};

		o.consume( EXPECT_N_CALLS_WRAPPER( 5, consumer ) )
		.fail( EXPECT_NO_CALL_WRAPPER( on_error ) );
	};

	run(
		o_in
		.group_by( EXPECT_N_CALLS_WRAPPER( 10, grouper ) )
		.consume( EXPECT_N_CALLS_WRAPPER( 2, consumer ) )
	);
}

TEST_F( ob_trans_group_by, hashable_key )
{
	auto o_in = q::rx::observable< int >::range( 1, 4, { queue } );

	std::vector< std::size_t > counter = { 0, 0 };
	std::vector< std::vector< int > > values{
		{ 2, 4 },
		{ 1, 3 }
	};

	auto grouper = [ ]( int i ) -> hashable
	{
		return hashable{ i % 2 };
	};

	auto consumer = [ &, this ]( hashable key, q::rx::observable< int > o )
	{
		auto consumer = [ &, key ]( int i )
		{
			EXPECT_EQ( i, values[ key.i ][ counter[ key.i ]++ ] );
		};

		o.consume( EXPECT_N_CALLS_WRAPPER( 2, consumer ) )
		.fail( EXPECT_NO_CALL_WRAPPER( on_error ) );
	};

	run(
		o_in
		.group_by( EXPECT_N_CALLS_WRAPPER( 4, grouper ) )
		.consume( EXPECT_N_CALLS_WRAPPER( 2, consumer ) )
	);
}

TEST_F( ob_trans_group_by, comparable_key )
{
	auto o_in = q::rx::observable< int >::range( 1, 4, { queue } );

	std::vector< std::size_t > counter = { 0, 0 };
	std::vector< std::vector< int > > values{
		{ 2, 4 },
		{ 1, 3 }
	};

	auto grouper = [ ]( int i ) -> comparable
	{
		return comparable{ i % 2 };
	};

	auto consumer = [ &, this ]( comparable key, q::rx::observable< int > o )
	{
		auto consumer = [ &, key ]( int i )
		{
			EXPECT_EQ( i, values[ key.i ][ counter[ key.i ]++ ] );
		};

		o.consume( EXPECT_N_CALLS_WRAPPER( 2, consumer ) )
		.fail( EXPECT_NO_CALL_WRAPPER( on_error ) );
	};

	run(
		o_in
		.group_by( EXPECT_N_CALLS_WRAPPER( 4, grouper ) )
		.consume( EXPECT_N_CALLS_WRAPPER( 2, consumer ) )
	);
}

TEST_F( ob_trans_group_by, hashable_key_async )
{
	auto o_in = q::rx::observable< int >::range( 1, 4, { queue } );

	std::vector< std::size_t > counter = { 0, 0 };
	std::vector< std::vector< int > > values{
		{ 2, 4 },
		{ 1, 3 }
	};

	auto tp_queue = this->tp_queue;

	auto grouper = [ tp_queue ]( int i )
	{
		return q::with( tp_queue, hashable{ i % 2 } );
	};

	auto consumer = [ &, this ]( hashable key, q::rx::observable< int > o )
	{
		auto consumer = [ &, key ]( int i )
		{
			EXPECT_EQ( i, values[ key.i ][ counter[ key.i ]++ ] );
		};

		o.consume( EXPECT_N_CALLS_WRAPPER( 2, consumer ) )
		.fail( EXPECT_NO_CALL_WRAPPER( on_error ) );
	};

	run(
		o_in
		.group_by( EXPECT_N_CALLS_WRAPPER( 4, grouper ) )
		.consume( EXPECT_N_CALLS_WRAPPER( 2, consumer ) )
	);
}

TEST_F( ob_trans_group_by, comparable_key_async )
{
	auto o_in = q::rx::observable< int >::range( 1, 4, { queue } );

	std::vector< std::size_t > counter = { 0, 0 };
	std::vector< std::vector< int > > values{
		{ 2, 4 },
		{ 1, 3 }
	};

	auto tp_queue = this->tp_queue;

	auto grouper = [ tp_queue ]( int i )
	{
		return q::with( tp_queue, comparable{ i % 2 } );
	};

	auto consumer = [ &, this ]( comparable key, q::rx::observable< int > o )
	{
		auto consumer = [ &, key ]( int i )
		{
			EXPECT_EQ( i, values[ key.i ][ counter[ key.i ]++ ] );
		};

		o.consume( EXPECT_N_CALLS_WRAPPER( 2, consumer ) )
		.fail( EXPECT_NO_CALL_WRAPPER( on_error ) );
	};

	run(
		o_in
		.group_by( EXPECT_N_CALLS_WRAPPER( 4, grouper ) )
		.consume( EXPECT_N_CALLS_WRAPPER( 2, consumer ) )
	);
}
