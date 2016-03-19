
#include <q-test/q-test.hpp>

Q_TEST_MAKE_SCOPE( then );

TEST_F( then, values_to_value )
{
	int i = 17;
	std::string s = "hello";

	run(
		q::with( queue, i, s )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( int i, std::string s ) -> long
		{
			return ( s[ 0 ] - s[ 1 ] ) * i;
		} ) )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( long value )
		{
			EXPECT_EQ( 3 * 17, value );
		} ) )
	);
}

TEST_F( then, tuple_to_value )
{
	int i = 17;
	std::string s = "hello";

	run(
		q::with( queue, i, s )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( std::tuple< int, std::string >&& values ) -> long
		{
			int& i = std::get< 0 >( values );
			std::string& s = std::get< 1 >( values );

			return ( s[ 0 ] - s[ 1 ] ) * i;
		} ) )
		.then( EXPECT_CALL_WRAPPER( spy )(
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
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ queue ]( int i, std::string s )
		{
			return q::with( queue, ( s[ 0 ] - s[ 1 ] ) * i );
		} ) )
		.then( EXPECT_CALL_WRAPPER( spy )(
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
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ queue ]( std::tuple< int, std::string >&& values )
		{
			int& i = std::get< 0 >( values );
			std::string& s = std::get< 1 >( values );

			return q::with( queue, ( s[ 0 ] - s[ 1 ] ) * i );
		} ) )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( long value )
		{
			EXPECT_EQ( 3 * 17, value );
		} ) )
	);
}

