
#include <q-test/q-test.hpp>

Q_TEST_MAKE_SCOPE( then );

TEST_F( then, values_to_value )
{
	int i = 17;
	std::string s = "hello";

	run(
		q::with( queue, i, s )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( int i, std::string s ) -> long
		{
			return ( s[ 0 ] - s[ 1 ] ) * i;
		} ) )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( long value )
		{
			EXPECT_EQ( 3 * 17, value );
		} ) )
	);
}

TEST_F( then, void_as_void )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( [ ]( ) { } ) )
	);
}

TEST_F( then, void_as_void_t )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( [ ]( q::void_t ) { } ) )
	);
}

TEST_F( then, void_as_void_t_const_ref )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( [ ]( const q::void_t& ) { } ) )
	);
}

TEST_F( then, void_as_void_t_ref_ref )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( [ ]( q::void_t&& ) { } ) )
	);
}

TEST_F( then, void_as_void_async )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
			[ this ]( )
			{
				return q::with( queue );
			}
		) )
	);
}

TEST_F( then, void_as_void_t_async )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
			[ this ]( q::void_t )
			{
				return q::with( queue );
			}
		) )
	);
}

TEST_F( then, void_as_void_t_const_ref_async )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
			[ this ]( const q::void_t& )
			{
				return q::with( queue );
			}
		) )
	);
}

TEST_F( then, void_as_void_t_ref_ref_async )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
			[ this ]( q::void_t&& )
			{
				return q::with( queue );
			}
		) )
	);
}

TEST_F( then, void_t_as_void )
{
	run(
		q::with( queue, q::void_t( ) )
		.then( EXPECT_CALL_WRAPPER( [ ]( ) { } ) )
	);
}

TEST_F( then, void_t_as_void_t )
{
	run(
		q::with( queue, q::void_t( ) )
		.then( EXPECT_CALL_WRAPPER( [ ]( q::void_t ) { } ) )
	);
}

TEST_F( then, void_t_as_void_t_const_ref )
{
	run(
		q::with( queue, q::void_t( ) )
		.then( EXPECT_CALL_WRAPPER( [ ]( const q::void_t& ) { } ) )
	);
}

TEST_F( then, void_t_as_void_t_ref_ref )
{
	run(
		q::with( queue, q::void_t( ) )
		.then( EXPECT_CALL_WRAPPER( [ ]( q::void_t&& ) { } ) )
	);
}

TEST_F( then, void_t_as_void_async )
{
	run(
		q::with( queue, q::void_t( ) )
		.then( EXPECT_CALL_WRAPPER(
			[ this ]( )
			{
				return q::with( queue );
			}
		) )
	);
}

TEST_F( then, void_t_as_void_t_async )
{
	run(
		q::with( queue, q::void_t( ) )
		.then( EXPECT_CALL_WRAPPER(
			[ this ]( q::void_t )
			{
				return q::with( queue );
			}
		) )
	);
}

TEST_F( then, void_t_as_void_t_const_ref_async )
{
	run(
		q::with( queue, q::void_t( ) )
		.then( EXPECT_CALL_WRAPPER(
			[ this ]( const q::void_t& )
			{
				return q::with( queue );
			}
		) )
	);
}

TEST_F( then, void_t_as_void_t_ref_ref_async )
{
	run(
		q::with( queue, q::void_t( ) )
		.then( EXPECT_CALL_WRAPPER(
			[ this ]( q::void_t&& )
			{
				return q::with( queue );
			}
		) )
	);
}

TEST_F( then, tuple_to_value )
{
	int i = 17;
	std::string s = "hello";

	run(
		q::with( queue, i, s )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( std::tuple< int, std::string >&& values ) -> long
		{
			int& i = std::get< 0 >( values );
			std::string& s = std::get< 1 >( values );

			return ( s[ 0 ] - s[ 1 ] ) * i;
		} ) )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( long value )
		{
			EXPECT_EQ( 3 * 17, value );
		} ) )
	);
}

TEST_F( then, values_to_promise )
{
	int i = 17;
	std::string s = "hello";

	auto queue = this->queue;

	run(
		q::with( queue, i, s )
		.then( EXPECT_CALL_WRAPPER(
		[ queue ]( int i, std::string s )
		{
			return q::with( queue, ( s[ 0 ] - s[ 1 ] ) * i );
		} ) )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( long value )
		{
			EXPECT_EQ( 3 * 17, value );
		} ) )
	);
}

TEST_F( then, tuple_to_promise )
{
	int i = 17;
	std::string s = "hello";

	auto queue = this->queue;

	run(
		q::with( queue, i, s )
		.then( EXPECT_CALL_WRAPPER(
		[ queue ]( std::tuple< int, std::string >&& values )
		{
			int& i = std::get< 0 >( values );
			std::string& s = std::get< 1 >( values );

			return q::with( queue, ( s[ 0 ] - s[ 1 ] ) * i );
		} ) )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( long value )
		{
			EXPECT_EQ( 3 * 17, value );
		} ) )
	);
}

