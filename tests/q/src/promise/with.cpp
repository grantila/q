
#include <q-test/q-test.hpp>
#include <q-test/expect.hpp>

Q_TEST_MAKE_SCOPE( with );

TEST_F( with, with_nothing )
{
	EVENTUALLY_EXPECT_RESOLUTION( q::with( queue ) );
}

TEST_F( with, with_one_argument )
{
	EVENTUALLY_EXPECT_EQ( q::with( queue, 5 ), 5 );
}

TEST_F( with, with_two_arguments )
{
	EVENTUALLY_EXPECT_EQ(
		q::with( queue, 5, 3.14 ), std::make_tuple( 5, 3.14 ) );
}

TEST_F( with, with_empty_tuple )
{
	EVENTUALLY_EXPECT_EQ(
		q::with( queue, std::make_tuple( ) ), std::make_tuple( ) );
}

TEST_F( with, with_one_value_tuple )
{
	EVENTUALLY_EXPECT_EQ(
		q::with( queue, std::make_tuple( 5 ) ), std::make_tuple( 5 ) );
}

TEST_F( with, with_two_values_tuple )
{
	EVENTUALLY_EXPECT_EQ(
		q::with( queue, std::make_tuple( 5, 3.14 ) ),
		std::make_tuple( 5, 3.14 ) );
}

TEST_F( with, with_empty_unique_promise )
{
	EVENTUALLY_EXPECT_RESOLUTION( q::with( queue, q::with( queue ) ) );
}

TEST_F( with, with_non_empty_unique_promise )
{
	EVENTUALLY_EXPECT_EQ(
		q::with( queue, q::with( queue, 5 ) ),
		5 );
}

TEST_F( with, with_empty_shared_promise )
{
	EVENTUALLY_EXPECT_RESOLUTION(
		q::with( queue, q::with( queue ).share( ) ) );
}

TEST_F( with, with_non_empty_shared_promise )
{
	EVENTUALLY_EXPECT_EQ(
		q::with( queue, q::with( queue, 5 ).share( ) ),
		5 );
}

TEST_F( with, with_char_pointer_to_string )
{
	run(
		q::with( queue, "test" )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( std::string&& s )
		{
			EXPECT_EQ( s, "test" );
		} ) )
	);
}
