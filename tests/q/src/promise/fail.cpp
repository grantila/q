
#include <q-test/q-test.hpp>

Q_TEST_MAKE_SCOPE( fail );
/*
	 *   * exception_ptr -> tuple< T... >
	 *   * exception_ptr -> P< tuple< T... > >
	 *   * E             -> tuple< T... >
	 *   * E             -> P< tuple< T... > >
*/

TEST_F( fail, exception_ptr_to_value )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( ) -> int
		{
			throw Error( );
		} ) )
		.then( EXPECT_NO_CALL( spy, int, int )( 4711 ) )
		.fail( EXPECT_CALL_WRAPPER( spy )(
		[ ]( std::exception_ptr e )
		{
			return 17;
		} ) )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( int value )
		{
			EXPECT_EQ( 17, value );
		} ) )
	);
}

TEST_F( fail, exception_ptr_to_promise )
{
	auto queue = this->queue;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( ) -> int
		{
			throw Error( );
		} ) )
		.then( EXPECT_NO_CALL( spy, int, int )( 4711 ) )
		.fail( EXPECT_CALL_WRAPPER( spy )(
		[ queue ]( std::exception_ptr e )
		{
			return q::with( queue, 17 );
		} ) )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( int value )
		{
			EXPECT_EQ( 17, value );
		} ) )
	);
}

TEST_F( fail, error_class_to_value )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( ) -> int
		{
			throw Error( );
		} ) )
		.then( EXPECT_NO_CALL( spy, int, int )( 4711 ) )
		.fail( EXPECT_CALL_WRAPPER( spy )(
		[ ]( Error& e )
		{
			return 17;
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER( spy )(
		[ ]( std::exception_ptr e )
		{
			return 18;
		} ) )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( int value )
		{
			EXPECT_EQ( 17, value );
		} ) )
	);
}

TEST_F( fail, error_class_to_promise )
{
	auto queue = this->queue;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( ) -> int
		{
			throw Error( );
		} ) )
		.then( EXPECT_NO_CALL( spy, int, int )( 4711 ) )
		.fail( EXPECT_CALL_WRAPPER( spy )(
		[ queue ]( Error& e )
		{
			return q::with( queue, 17 );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER( spy )(
		[ queue ]( std::exception_ptr e )
		{
			return q::with( queue, 18 );
		} ) )
		.then( EXPECT_CALL_WRAPPER( spy )(
		[ ]( int value )
		{
			EXPECT_EQ( 17, value );
		} ) )
	);
}

