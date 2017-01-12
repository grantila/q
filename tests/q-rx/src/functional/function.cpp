
#include "../q-rx-test.hpp"

Q_TEST_MAKE_SCOPE( functional_function );

TEST_F( functional_function, append_result_no_arg_void_result )
{
	auto fn = [ ]( )
	{
		return;
	};

	auto wrapped = q::rx::f::append_result( fn );

	EXPECT_EQ( wrapped( ), std::make_tuple( ) );
}

TEST_F( functional_function, append_result_no_arg_single_result )
{
	auto fn = [ ]( )
	{
		return 5;
	};

	auto wrapped = q::rx::f::append_result( fn );

	EXPECT_EQ( wrapped( ), std::make_tuple( 5 ) );
}

TEST_F( functional_function, append_result_no_arg_tuple_result )
{
	auto fn = [ ]( )
	{
		return std::make_tuple( long( 10 ) );
	};

	auto wrapped = q::rx::f::append_result( fn );

	EXPECT_EQ( wrapped( ), std::make_tuple( long( 10 ) ) );
}

TEST_F( functional_function, append_result_no_arg_empty_tuple_result )
{
	auto fn = [ ]( )
	{
		return std::make_tuple( );
	};

	auto wrapped = q::rx::f::append_result( fn );

	EXPECT_EQ( wrapped( ), std::make_tuple( ) );
}

TEST_F( functional_function, append_result_no_arg_promise_empty_result )
{
	auto queue = this->queue;

	auto fn = [ queue ]( )
	{
		return q::with( queue );
	};

	auto wrapped = q::rx::f::append_result( fn );

	EVENTUALLY_EXPECT_EQ( wrapped( ), std::make_tuple( ) );
}

TEST_F( functional_function, append_result_no_arg_promise_multi_result )
{
	auto queue = this->queue;

	auto fn = [ queue ]( )
	{
		return q::with( queue, long( 10 ) );
	};

	auto wrapped = q::rx::f::append_result( fn );

	EVENTUALLY_EXPECT_EQ( wrapped( ), std::make_tuple( long( 10 ) ) );
}

TEST_F( functional_function, append_result_one_arg_void_result )
{
	auto fn = [ ]( int i )
	{
		return;
	};

	auto wrapped = q::rx::f::append_result( fn );

	EXPECT_EQ( wrapped( 5 ), std::make_tuple( 5 ) );
}

TEST_F( functional_function, append_result_one_arg_single_result )
{
	auto fn = [ ]( int i )
	{
		return std::to_string( i );
	};

	auto wrapped = q::rx::f::append_result( fn );

	EXPECT_EQ( wrapped( 5 ), std::make_tuple( 5, "5" ) );
}

TEST_F( functional_function, append_result_one_arg_tuple_result )
{
	auto fn = [ ]( int i )
	{
		return std::make_tuple( std::to_string( i ), long( 10 ) );
	};

	auto wrapped = q::rx::f::append_result( fn );

	EXPECT_EQ( wrapped( 5 ), std::make_tuple( 5, "5", long( 10 ) ) );
}

TEST_F( functional_function, append_result_one_arg_empty_tuple_result )
{
	auto fn = [ ]( int i )
	{
		return std::make_tuple( );
	};

	auto wrapped = q::rx::f::append_result( fn );

	EXPECT_EQ( wrapped( 5 ), std::make_tuple( 5 ) );
}

TEST_F( functional_function, append_result_one_arg_promise_empty_result )
{
	auto queue = this->queue;

	auto fn = [ queue ]( int i )
	{
		return q::with( queue );
	};

	auto wrapped = q::rx::f::append_result( fn );

	EVENTUALLY_EXPECT_EQ( wrapped( 5 ), std::make_tuple( 5 ) );
}

TEST_F( functional_function, append_result_one_arg_promise_multi_result )
{
	auto queue = this->queue;

	auto fn = [ queue ]( int i )
	{
		return q::with( queue, std::to_string( i ), long( 10 ) );
	};

	auto wrapped = q::rx::f::append_result( fn );

	EVENTUALLY_EXPECT_EQ(
		wrapped( 5 ),
		std::make_tuple( 5, "5", long( 10 ) )
	);
}
