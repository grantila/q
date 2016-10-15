
#include <q-test/q-test.hpp>

Q_TEST_MAKE_SCOPE( tap_error );

TEST_F( tap_error, exception_ptr_to_void )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> int
		{
			Q_THROW( Error( ) );
		} ) )
		.tap_error( EXPECT_CALL_WRAPPER(
		[ ]( std::exception_ptr e )
		{
			EXPECT_THROW( std::rethrow_exception( e ), Error );
		} ) )
		.fail( EXPECT_CALL( int, Error )( 0 ) )
	);
}

TEST_F( tap_error, exception_ptr_to_promise )
{
	auto queue = this->queue;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> int
		{
			Q_THROW( Error( ) );
		} ) )
		.tap_error( EXPECT_CALL_WRAPPER(
		[ queue ]( std::exception_ptr e )
		{
			EXPECT_THROW( std::rethrow_exception( e ), Error );

			return q::with( queue );
		} ) )
		.fail( EXPECT_CALL( int, Error )( 0 ) )
	);
}

TEST_F( tap_error, error_class_to_value )
{
	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> int
		{
			Q_THROW( Error( ) );
		} ) )
		.tap_error( EXPECT_CALL_WRAPPER( [ ]( Error& ) { } ) )
		.fail( EXPECT_CALL( int, Error )( 0 ) )
	);
}


TEST_F( tap_error, error_class_to_promise )
{
	auto queue = this->queue;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ ]( ) -> int
		{
			Q_THROW( Error( ) );
		} ) )
		.tap_error( EXPECT_CALL_WRAPPER(
		[ queue ]( Error& e )
		{
			return q::with( queue );
		} ) )
		.fail( EXPECT_CALL( int, Error )( 0 ) )
	);
}

TEST_F( tap_error, exception_ptr_throwing_handler )
{
	float magic = 3.14;

	run(
		q::with( queue )
		.then( EXPECT_CALL_WRAPPER(
		[ magic ]( ) -> int
		{
			Q_THROW( Error( ), std::move( magic ) );
		} ) )
		.tap_error( EXPECT_CALL_WRAPPER(
		[ magic ]( std::exception_ptr e )
		{
			Q_THROW( Error( ), magic * 2 );
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
				EXPECT_FLOAT_EQ( magic * 2, current_magic );
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

TEST_F( tap_error, exception_ptr_throwing_handler_promise_sync )
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
		.fail( EXPECT_CALL_WRAPPER(
		[ magic ]( std::exception_ptr e )
		-> q::promise< std::tuple< int > >
		{
			Q_THROW( Error( ), magic * 2 );
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
				EXPECT_FLOAT_EQ( magic * 2, current_magic );
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

TEST_F( tap_error, exception_ptr_throwing_handler_promise_async )
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
		.fail( EXPECT_CALL_WRAPPER( (
		[ queue, magic ]( std::exception_ptr e )
		-> q::promise< std::tuple< int > >
		{
			return q::reject< q::arguments< int > >(
				queue,
				::q::add_exception_properties(
					Error( ),
					magic * 2
				)
			);
		} ) ) )
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
				EXPECT_FLOAT_EQ( magic * 2, current_magic );
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
