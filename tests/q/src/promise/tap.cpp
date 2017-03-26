
#include "../core.hpp"

Q_TEST_MAKE_SCOPE( tap );

TEST_F( tap, values_to_value )
{
	int i = 17;

	run(
		q::with( queue, i )
		.tap( EXPECT_CALL_WRAPPER(
		[ ]( int value )
		{
			EXPECT_EQ( 17, value );
		} ) )
		.then( EXPECT_CALL_WRAPPER( [ ]( int value )
		{
			EXPECT_EQ( 17, value );
		} ) )
	);
}

TEST_F( tap, empty )
{
	run(
		q::with( queue )
		.tap( EXPECT_CALL_WRAPPER( [ ]( ) { } ) )
	);
}

TEST_F( tap, void_as_void_t )
{
	run(
		q::with( queue )
		.tap( EXPECT_CALL_WRAPPER( [ ]( q::void_t ) { } ) )
	);
}

TEST_F( tap, void_as_void_t_ref_ref )
{
	run(
		q::with( queue )
		.tap( EXPECT_CALL_WRAPPER( [ ]( q::void_t&& ) { } ) )
	);
}

TEST_F( tap, tuple_to_value )
{
	int i = 17;
	std::string s = "hello";

	run(
		q::with( queue, i, s )
		.tap( EXPECT_CALL_WRAPPER(
		[ ]( const std::tuple< int, std::string >& values )
		{
			const int& i = std::get< 0 >( values );
			const std::string& s = std::get< 1 >( values );
			EXPECT_EQ( 17, i );
			EXPECT_EQ( "hello", s );
		} ) )
	);
}

TEST_F( tap, values_to_promise )
{
	int i = 17;
	std::string s = "hello";

	auto queue = this->queue;

	run(
		q::with( queue, i, s )
		.tap( EXPECT_CALL_WRAPPER(
		[ queue ]( int i, std::string s )
		{
			EXPECT_EQ( 17, i );
			EXPECT_EQ( "hello", s );

			return q::with( queue );
		} ) )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( int i, std::string s )
		{
			EXPECT_EQ( 17, i );
			EXPECT_EQ( "hello", s );
		} ) )
	);
}

TEST_F( tap, tuple_to_promise )
{
	int i = 17;
	std::string s = "hello";

	auto queue = this->queue;

	run(
		q::with( queue, i, s )
		.tap( EXPECT_CALL_WRAPPER(
		[ queue ]( const std::tuple< int, std::string >& values )
		{
			const int& i = std::get< 0 >( values );
			const std::string& s = std::get< 1 >( values );

			EXPECT_EQ( 17, i );
			EXPECT_EQ( "hello", s );

			return q::with( queue );
		} ) )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( std::tuple< int, std::string >&& values )
		{
			int& i = std::get< 0 >( values );
			std::string& s = std::get< 1 >( values );

			EXPECT_EQ( 17, i );
			EXPECT_EQ( "hello", s );
		} ) )
	);
}
