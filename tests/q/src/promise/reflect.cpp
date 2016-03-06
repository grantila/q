
#include <q-test/q-test.hpp>

Q_TEST_MAKE_SCOPE( Reflect );

TEST_F( Reflect, ReflectTupleWithZeroElements )
{
	run(
		q::with( queue )
		.reflect_tuple( )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::expect< std::tuple< > >&& exp )
		{
			EXPECT_FALSE( exp.has_exception( ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER( spy )(
		[ ]( std::exception_ptr )
		{
		} ) )
	);
}

TEST_F( Reflect, ReflectTupleWithOneElement )
{
	run(
		q::with( queue, 17 )
		.reflect_tuple( )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::expect< std::tuple< int > >&& exp )
		{
			EXPECT_FALSE( exp.has_exception( ) );
			EXPECT_EQ( 17, std::get< 0 >( exp.get( ) ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER( spy )(
		[ ]( std::exception_ptr )
		{
		} ) )
	);
}

TEST_F( Reflect, ReflectTupleWithTwoElements )
{
	run(
		q::with( queue, 17, 3.14f )
		.reflect_tuple( )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::expect< std::tuple< int, float > >&& exp )
		{
			EXPECT_FALSE( exp.has_exception( ) );
			EXPECT_EQ( 17, std::get< 0 >( exp.get( ) ) );
			EXPECT_FLOAT_EQ( (float)3.14, std::get< 1 >( exp.get( ) ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER( spy )(
		[ ]( std::exception_ptr )
		{
		} ) )
	);

}

TEST_F( Reflect, ReflectTupleWithZeroElementsWithException )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( )
		{
			throw Error( );
		} ) )
		.reflect_tuple( )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::expect< std::tuple< > >&& exp )
		{
			EXPECT_TRUE( exp.has_exception( ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER( spy )(
		[ ]( std::exception_ptr )
		{
		} ) )
	);
}

TEST_F( Reflect, ReflectTupleWithOneElementWithException )
{
	run(
		q::with( queue, 17 )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( int i ) -> int
		{
			EXPECT_EQ( 17, i );
			throw Error( );
		} ) )
		.reflect_tuple( )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::expect< std::tuple< int > >&& exp )
		{
			EXPECT_TRUE( exp.has_exception( ) );
			std::rethrow_exception( exp.exception( ) );
		} ) )
		.fail( EXPECT_CALL_WRAPPER( spy )(
		[ ]( Error& )
		{
		} ) )
	);
}

TEST_F( Reflect, ReflectTupleWithTwoElementsWithException )
{
	run(
		q::with( queue, 17, 3.14f )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( int i, float f ) -> std::tuple< int, float >
		{
			EXPECT_EQ( 17, i );
			EXPECT_FLOAT_EQ( (float)3.14, f );
			throw Error( );
		} ) )
		.reflect_tuple( )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::expect< std::tuple< int, float > >&& exp )
		{
			EXPECT_TRUE( exp.has_exception( ) );
			std::rethrow_exception( exp.exception( ) );
		} ) )
		.fail( EXPECT_CALL_WRAPPER( spy )(
		[ ]( Error& )
		{
		} ) )
	);

}

TEST_F( Reflect, ReflectWithZeroElements )
{
	run(
		q::with( queue )
		.reflect( )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::expect< >&& exp )
		{
			EXPECT_FALSE( exp.has_exception( ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER( spy )(
		[ ]( std::exception_ptr )
		{
		} ) )
	);
}

TEST_F( Reflect, ReflectWithOneElement )
{
	run(
		q::with( queue, 17 )
		.reflect( )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::expect< int >&& exp )
		{
			EXPECT_FALSE( exp.has_exception( ) );
			EXPECT_EQ( 17, exp.get( ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER( spy )(
		[ ]( std::exception_ptr )
		{
		} ) )
	);
}

TEST_F( Reflect, ReflectWithTwoElements )
{
	run(
		q::with( queue, 17, 3.14f )
		.reflect( )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::expect< std::tuple< int, float > >&& exp )
		{
			EXPECT_FALSE( exp.has_exception( ) );
			EXPECT_EQ( 17, std::get< 0 >( exp.get( ) ) );
			EXPECT_FLOAT_EQ( (float)3.14, std::get< 1 >( exp.get( ) ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER( spy )(
		[ ]( std::exception_ptr )
		{
		} ) )
	);

}

TEST_F( Reflect, ReflectWithZeroElementsWithException )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( )
		{
			throw Error( );
		} ) )
		.reflect( )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::expect< >&& exp )
		{
			EXPECT_TRUE( exp.has_exception( ) );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER( spy )(
		[ ]( std::exception_ptr )
		{
		} ) )
	);
}

TEST_F( Reflect, ReflectWithOneElementWithException )
{
	run(
		q::with( queue, 17 )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( int i ) -> int
		{
			EXPECT_EQ( 17, i );
			throw Error( );
		} ) )
		.reflect( )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::expect< int >&& exp )
		{
			EXPECT_TRUE( exp.has_exception( ) );
			std::rethrow_exception( exp.exception( ) );
		} ) )
		.fail( EXPECT_CALL_WRAPPER( spy )(
		[ ]( Error& )
		{
		} ) )
	);
}

TEST_F( Reflect, ReflectWithTwoElementsWithException )
{
	run(
		q::with( queue, 17, 3.14f )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( int i, float f ) -> std::tuple< int, float >
		{
			EXPECT_EQ( 17, i );
			EXPECT_FLOAT_EQ( (float)3.14, f );
			throw Error( );
		} ) )
		.reflect( )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( q::expect< std::tuple< int, float > >&& exp )
		{
			EXPECT_TRUE( exp.has_exception( ) );
			std::rethrow_exception( exp.exception( ) );
		} ) )
		.fail( EXPECT_CALL_WRAPPER( spy )(
		[ ]( Error& )
		{
		} ) )
	);

}
