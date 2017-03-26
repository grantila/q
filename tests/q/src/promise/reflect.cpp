
#include "../core.hpp"

Q_TEST_MAKE_SCOPE( reflect );

TEST_F( reflect, reflect_tuple_with_zero_elements )
{
	run(
		q::with( queue )
		.reflect_tuple( )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( q::expect< std::tuple< > >&& exp )
		{
			EXPECT_FALSE( exp.has_exception( ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr )
		{
		} ) )
	);
}

TEST_F( reflect, reflect_tuple_with_one_element )
{
	run(
		q::with( queue, 17 )
		.reflect_tuple( )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( q::expect< std::tuple< int > >&& exp )
		{
			EXPECT_FALSE( exp.has_exception( ) );
			EXPECT_EQ( 17, std::get< 0 >( exp.get( ) ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr )
		{
		} ) )
	);
}

TEST_F( reflect, reflect_tuple_with_two_elements )
{
	run(
		q::with( queue, 17, 3.14f )
		.reflect_tuple( )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( q::expect< std::tuple< int, float > >&& exp )
		{
			EXPECT_FALSE( exp.has_exception( ) );
			EXPECT_EQ( 17, std::get< 0 >( exp.get( ) ) );
			EXPECT_FLOAT_EQ( (float)3.14, std::get< 1 >( exp.get( ) ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr )
		{
		} ) )
	);

}

TEST_F( reflect, reflect_tuple_with_zero_elements_with_exception )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( )
		{
			Q_THROW( Error( ) );
		} ) )
		.reflect_tuple( )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( q::expect< std::tuple< > >&& exp )
		{
			EXPECT_TRUE( exp.has_exception( ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr )
		{
		} ) )
	);
}

TEST_F( reflect, reflect_tuple_with_one_element_with_exception )
{
	run(
		q::with( queue, 17 )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( int i ) -> int
		{
			EXPECT_EQ( 17, i );
			Q_THROW( Error( ) );
		} ) )
		.reflect_tuple( )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( q::expect< std::tuple< int > >&& exp )
		{
			EXPECT_TRUE( exp.has_exception( ) );
			std::rethrow_exception( exp.exception( ) );
		} ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( Error& )
		{
		} ) )
	);
}

TEST_F( reflect, reflect_tuple_with_two_elements_with_exception )
{
	run(
		q::with( queue, 17, 3.14f )
		.then( EXPECT_CALL_WRAPPER( (
		[ ]( int i, float f ) -> std::tuple< int, float >
		{
			EXPECT_EQ( 17, i );
			EXPECT_FLOAT_EQ( (float)3.14, f );
			Q_THROW( Error( ) );
		} ) ) )
		.reflect_tuple( )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( q::expect< std::tuple< int, float > >&& exp )
		{
			EXPECT_TRUE( exp.has_exception( ) );
			std::rethrow_exception( exp.exception( ) );
		} ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( Error& )
		{
		} ) )
	);

}

TEST_F( reflect, reflect_with_zero_elements )
{
	run(
		q::with( queue )
		.reflect( )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( q::expect< >&& exp )
		{
			EXPECT_FALSE( exp.has_exception( ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr )
		{
		} ) )
	);
}

TEST_F( reflect, reflect_with_one_element )
{
	run(
		q::with( queue, 17 )
		.reflect( )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( q::expect< int >&& exp )
		{
			EXPECT_FALSE( exp.has_exception( ) );
			EXPECT_EQ( 17, exp.get( ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr )
		{
		} ) )
	);
}

TEST_F( reflect, reflect_with_two_elements )
{
	run(
		q::with( queue, 17, 3.14f )
		.reflect( )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( q::expect< std::tuple< int, float > >&& exp )
		{
			EXPECT_FALSE( exp.has_exception( ) );
			EXPECT_EQ( 17, std::get< 0 >( exp.get( ) ) );
			EXPECT_FLOAT_EQ( (float)3.14, std::get< 1 >( exp.get( ) ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr )
		{
		} ) )
	);

}

TEST_F( reflect, reflect_with_zero_elements_with_exception )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( )
		{
			Q_THROW( Error( ) );
		} ) )
		.reflect( )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( q::expect< >&& exp )
		{
			EXPECT_TRUE( exp.has_exception( ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr )
		{
		} ) )
	);
}

TEST_F( reflect, reflect_with_one_element_with_exception )
{
	run(
		q::with( queue, 17 )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( int i ) -> int
		{
			EXPECT_EQ( 17, i );
			Q_THROW( Error( ) );
		} ) )
		.reflect( )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( q::expect< int >&& exp )
		{
			EXPECT_TRUE( exp.has_exception( ) );
			std::rethrow_exception( exp.exception( ) );
		} ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( Error& )
		{
		} ) )
	);
}

TEST_F( reflect, reflect_with_two_elements_with_exception )
{
	run(
		q::with( queue, 17, 3.14f )
		.then( EXPECT_CALL_WRAPPER( (
		[ ]( int i, float f ) -> std::tuple< int, float >
		{
			EXPECT_EQ( 17, i );
			EXPECT_FLOAT_EQ( (float)3.14, f );
			Q_THROW( Error( ) );
		} ) ) )
		.reflect( )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( q::expect< std::tuple< int, float > >&& exp )
		{
			EXPECT_TRUE( exp.has_exception( ) );
			std::rethrow_exception( exp.exception( ) );
		} ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( Error& )
		{
		} ) )
	);

}
