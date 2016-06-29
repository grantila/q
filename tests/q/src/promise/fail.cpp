
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
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> int
		{
			Q_THROW( Error( ) );
		} ) )
		.then( EXPECT_NO_CALL( int, int )( 4711 ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( std::exception_ptr e )
		{
			return 17;
		} ) )
		.then( EXPECT_CALL_WRAPPER(
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
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> int
		{
			Q_THROW( Error( ) );
		} ) )
		.then( EXPECT_NO_CALL( int, int )( 4711 ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ queue ]( std::exception_ptr e )
		{
			return q::with( queue, 17 );
		} ) )
		.then( EXPECT_CALL_WRAPPER(
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
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> int
		{
			Q_THROW( Error( ) );
		} ) )
		.then( EXPECT_NO_CALL( int, int )( 4711 ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( Error& e )
		{
			return 17;
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr e )
		{
			return 18;
		} ) )
		.then( EXPECT_CALL_WRAPPER(
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
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> int
		{
			Q_THROW( Error( ) );
		} ) )
		.then( EXPECT_NO_CALL( int, int )( 4711 ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ queue ]( Error& e )
		{
			return q::with( queue, 17 );
		} ) )
		.fail( EXPECT_NO_CALL_WRAPPER(
		[ queue ]( std::exception_ptr e )
		{
			return q::with( queue, 18 );
		} ) )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( int value )
		{
			EXPECT_EQ( 17, value );
		} ) )
	);
}

TEST_F( fail, exception_ptr_fail_twice )
{
	float magic = 3.14;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ magic ]( ) -> int
		{
			Q_THROW( Error( ), std::move( magic ) );
		} ) )
		.then( EXPECT_NO_CALL( int, int )( 4711 ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( std::exception_ptr e )
		-> int
		{
			std::rethrow_exception( e );
		} ) )
		.fail( EXPECT_CALL_WRAPPER(
		[ magic ]( std::exception_ptr e )
		-> int
		{
			try
			{
				std::rethrow_exception( e );
			}
			catch ( const Error& error )
			{
				auto magic_info = error.get_info< float >( );
				auto current_magic = magic_info->get( );
				EXPECT_FLOAT_EQ( magic, current_magic );
			}
			catch ( ... )
			{
				Q_THROW( Error( ), "Shouldn't happen" );
			}
			return 17;
		} ) )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( int value )
		{
			EXPECT_EQ( 17, value );
		} ) )
	);
}

TEST_F( fail, exception_ptr_fail_twice_on_promises )
{
	float magic = 3.14;

	q::queue_ptr queue = this->queue;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ magic ]( ) -> q::promise< std::tuple< int > >
		{
			Q_THROW( Error( ), std::move( magic ) );
		} ) )
		.then( EXPECT_NO_CALL_WRAPPER(
			[ queue ]( int )
			-> q::promise< std::tuple< int > >
			{
				return q::with( queue, 4711 );
			}
		) )
		.fail( EXPECT_CALL_WRAPPER(
		[ ]( std::exception_ptr e )
		-> q::promise< std::tuple< int > >
		{
			std::rethrow_exception( e );
		} ) )
		.fail( EXPECT_CALL_WRAPPER( (
		[ magic, queue ]( std::exception_ptr e )
		-> q::promise< std::tuple< int > >
		{
			try
			{
				std::rethrow_exception( e );
			}
			catch ( const Error& error )
			{
				auto magic_info = error.get_info< float >( );
				auto current_magic = magic_info->get( );
				EXPECT_FLOAT_EQ( magic, current_magic );
			}
			catch ( ... )
			{
				Q_THROW( Error( ), "Shouldn't happen" );
			}
			return q::with( queue, 17 );
		} ) ) )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( int value )
		{
			EXPECT_EQ( 17, value );
		} ) )
	);
}
