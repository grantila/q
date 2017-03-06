
#include <q/channel.hpp>
#include <q/function.hpp>

#include <q-test/q-test.hpp>
#include <q-test/expect.hpp>

Q_TEST_MAKE_SCOPE( channel );

Q_MAKE_SIMPLE_EXCEPTION( test_exception );

TEST_F( channel, create )
{
	q::channel< int > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );
}

TEST_F( channel, zero_types )
{
	q::channel< > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	EXPECT_TRUE( writable.write( ) );
	EXPECT_TRUE( writable.write( std::make_tuple( ) ) );
	EXPECT_TRUE( writable.write( q::void_t( ) ) );
	EXPECT_TRUE( writable.write( std::make_tuple( q::void_t( ) ) ) );
	writable.close( );

	auto promise = readable.read( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( )
		{
			return readable.read( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( std::tuple< >&& )
		{
			return readable.read( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ ]( ) { }
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ ]( q::void_t&& ) { }
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ ]( std::tuple< q::void_t >&& ) { }
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( q::void_t&& )
		{
			return readable.read( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( std::tuple< q::void_t >&& )
		{
			return readable.read( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( ) { }
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ ]( q::channel_closed_exception& ) { }
	) )
	.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr e ) {
			std::cout << q::stream_exception( e ) << std::endl;
		}
	) );

	run( std::move( promise ) );
}

TEST_F( channel, one_type )
{
	q::channel< int > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	EXPECT_TRUE( writable.write( 17 ) );
	EXPECT_TRUE( writable.write( std::make_tuple< int >( 4711 ) ) );
	writable.close( );

	auto promise = readable.read( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 17 );

			return readable.read( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 4711 );

			return readable.read( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( int ) { }
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ ]( q::channel_closed_exception& ) { }
	) )
	.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr ) { }
	) );

	run( std::move( promise ) );
}

TEST_F( channel, two_types )
{
	q::channel< int, std::string > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	EXPECT_TRUE( writable.write( 17, "hello" ) );
	EXPECT_TRUE( writable.write( std::make_tuple( 4711, "world" ) ) );
	writable.close( );

	auto promise = readable.read( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value, std::string&& s )
		{
			EXPECT_EQ( 17, value );
			EXPECT_EQ( "hello", s );

			return readable.read( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value, std::string s )
		{
			EXPECT_EQ( 4711, value );
			EXPECT_EQ( "world", s );

			return readable.read( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( int, std::string&& ) { }
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ ]( q::channel_closed_exception& ) { }
	) )
	.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr ) { }
	) );

	run( std::move( promise ) );
}

TEST_F( channel, auto_close_on_readable_destruction )
{
	auto channel_creator = [ this ]( ) -> q::readable< int >
	{
		q::channel< int > ch( queue, 5 );

		auto readable = ch.get_readable( );
		auto writable = ch.get_writable( );

		EXPECT_TRUE( writable.write( 17 ) );
		EXPECT_TRUE( writable.write( 4711 ) );

		return readable;
	};

	auto readable = channel_creator( );

	auto promise = readable.read( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 17 );

			return readable.read( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 4711 );

			return readable.read( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( int ) { }
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ ]( q::channel_closed_exception& ) { }
	) )
	.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr ) { }
	) );

	run( std::move( promise ) );
}

TEST_F( channel, auto_close_on_writable_destruction )
{
	auto channel_creator = [ this ]( )
	-> std::tuple< q::promise< >, q::writable< int > >
	{
		q::channel< int > ch( queue, 5 );

		auto readable = ch.get_readable( );
		auto writable = ch.get_writable( );

		auto promise = readable.read( )
		.then( EXPECT_NO_CALL_WRAPPER(
			[ ]( int ) { }
		) )
		.fail( EXPECT_CALL_WRAPPER(
			[ ]( q::channel_closed_exception& ) { }
		) )
		.fail( EXPECT_NO_CALL_WRAPPER(
			[ ]( std::exception_ptr ) { }
		) );

		return std::make_tuple(
			std::move( promise ), std::move( writable ) );
	};

	auto tup = channel_creator( );

	auto promise = std::move( std::get< 0 >( tup ) );
	auto writable = std::move( std::get< 1 >( tup ) );

	EXPECT_FALSE( writable.write( 17 ) );
	EXPECT_THROW( writable.ensure_write( 17 ), q::channel_closed_exception );

	run( std::move( promise ) );
}

TEST_F( channel, channel_empty_promise_specialization )
{
	typedef q::promise< > promise_type;

	q::channel< promise_type > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	EXPECT_TRUE( writable.write( ) );
	EXPECT_TRUE( writable.write( std::make_tuple< >( ) ) );
	EXPECT_TRUE( writable.write( q::void_t( ) ) );
	EXPECT_TRUE( writable.write( std::make_tuple( q::void_t( ) ) ) );
	writable.close( );

	auto reader = EXPECT_N_CALLS_WRAPPER( 4,
		[ &readable ]( )
		{
			return readable.read( );
		}
	);

	auto promise = readable.read( )
	.then( reader )
	.then( reader )
	.then( reader )
	.then( reader )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( ) { }
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ ]( q::channel_closed_exception& ) { }
	) )
	.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr ) { }
	) );

	run( std::move( promise ) );
}

TEST_F( channel, channel_non_empty_promise_specialization )
{
	typedef q::promise< int > promise_type;

	q::channel< promise_type > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	EXPECT_TRUE( writable.write( 17 ) );
	EXPECT_TRUE( writable.write( std::make_tuple< int >( 4711 ) ) );
	writable.close( );

	auto promise = readable.read( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 17 );

			return readable.read( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 4711 );

			return readable.read( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( int ) { }
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ ]( q::channel_closed_exception& ) { }
	) )
	.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr ) { }
	) );

	run( std::move( promise ) );
}

TEST_F( channel, channel_promise_specialization_rejection )
{
	typedef q::promise< int > promise_type;

	q::channel< promise_type > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	auto rejected_promise = q::make_promise( queue, [ ]( ) -> int
	{
		Q_THROW( test_exception( ) );
	} );

	EXPECT_TRUE( writable.write( 5 ) );
	EXPECT_TRUE( writable.write( std::move( rejected_promise ) ) );
	EXPECT_TRUE( writable.write( 17 ) );
	writable.close( );

	auto promise = readable.read( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 5 );

			return readable.read( );
		}
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ &readable ]( const test_exception& )
		{
			return readable.read( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ this ]( int )
		{
			return q::with( queue, 5 );
		}
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ &readable ]( const test_exception& )
		{
			return readable.read( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( int ) { }
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ ]( const test_exception& ) { }
	) )
	.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr e )
		{
			std::cerr
				<< "Shouldn't end up here: "
				<< q::stream_exception( e )
				<< std::endl;
		}
	) );

	run( std::move( promise ) );
}

TEST_F( channel, channel_empty_shared_promise_specialization )
{
	typedef q::shared_promise< > promise_type;

	q::channel< promise_type > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	EXPECT_TRUE( writable.write( ) );
	EXPECT_TRUE( writable.write( std::make_tuple< >( ) ) );
	writable.close( );

	auto promise = readable.read( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( )
		{
			return readable.read( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( )
		{
			return readable.read( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( ) { }
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ ]( q::channel_closed_exception& ) { }
	) )
	.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr ) { }
	) );

	run( std::move( promise ) );
}

TEST_F( channel, channel_non_empty_shared_promise_specialization )
{
	typedef q::shared_promise< int > promise_type;

	q::channel< promise_type > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	EXPECT_TRUE( writable.write( 17 ) );
	EXPECT_TRUE( writable.write( std::make_tuple< int >( 4711 ) ) );
	writable.close( );

	auto promise = readable.read( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 17 );

			return readable.read( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 4711 );

			return readable.read( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( int ) { }
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ ]( q::channel_closed_exception& ) { }
	) )
	.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr ) { }
	) );

	run( std::move( promise ) );
}

TEST_F( channel, channel_shared_promise_specialization_rejection )
{
	typedef q::shared_promise< int > promise_type;

	q::channel< promise_type > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	auto rejected_promise = q::make_promise( queue, [ ]( ) -> int
	{
		Q_THROW( test_exception( ) );
	} ).share( );

	EXPECT_TRUE( writable.write( 5 ) );
	EXPECT_TRUE( writable.write( std::move( rejected_promise ) ) );
	EXPECT_TRUE( writable.write( 17 ) );
	writable.close( );

	auto promise = readable.read( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 5 );

			return readable.read( );
		}
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ &readable ]( const test_exception& )
		{
			return readable.read( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ this ]( int )
		{
			return q::with( queue, 5 );
		}
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ &readable ]( const test_exception& )
		{
			return readable.read( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( int ) { }
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ ]( const test_exception& ) { }
	) )
	.fail( EXPECT_NO_CALL_WRAPPER(
		[ ]( std::exception_ptr e )
		{
			std::cerr
				<< "Shouldn't end up here: "
				<< q::stream_exception( e )
				<< std::endl;
		}
	) );

	run( std::move( promise ) );
}

TEST_F( channel, fast_read_zero_types )
{
	q::channel< > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	EXPECT_TRUE( writable.write( ) );
	EXPECT_TRUE( writable.write( ) );
	writable.close( );

	auto on_value = [ ]( ) { };
	auto on_closed = [ ]( ) { };

	auto promise = readable.read(
		EXPECT_CALL_WRAPPER( on_value ),
		EXPECT_NO_CALL_WRAPPER( on_closed )
	)
	.then( EXPECT_CALL_WRAPPER( ( [ = ]( bool ) mutable
	{
		return readable.read(
			EXPECT_CALL_WRAPPER( on_value ),
			EXPECT_NO_CALL_WRAPPER( on_closed )
		);
	} ) ) )
	.then( EXPECT_CALL_WRAPPER( ( [ = ]( bool ) mutable
	{
		return readable.read(
			EXPECT_NO_CALL_WRAPPER( on_value ),
			EXPECT_CALL_WRAPPER( on_closed )
		);
	} ) ) );

	run( std::move( promise ) );
}

TEST_F( channel, fast_read_one_type )
{
	q::channel< int > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	std::vector< int > expected{ 17, 4711 };
	std::size_t counter = 0;

	EXPECT_TRUE( writable.write( expected[ 0 ] ) );
	EXPECT_TRUE( writable.write( expected[ 1 ] ) );
	writable.close( );

	auto on_value = [ & ]( int i )
	{
		EXPECT_EQ( expected[ counter++ ], i );
	};
	auto on_closed = [ ]( ) { };

	auto promise = readable.read(
		EXPECT_CALL_WRAPPER( on_value ),
		EXPECT_NO_CALL_WRAPPER( on_closed )
	)
	.then( EXPECT_CALL_WRAPPER( [ = ]( bool ) mutable
	{
		return readable.read(
			EXPECT_CALL_WRAPPER( on_value ),
			EXPECT_NO_CALL_WRAPPER( on_closed )
		);
	} ) )
	.then( EXPECT_CALL_WRAPPER( [ = ]( bool ) mutable
	{
		return readable.read(
			EXPECT_NO_CALL_WRAPPER( on_value ),
			EXPECT_CALL_WRAPPER( on_closed )
		);
	} ) );

	run( std::move( promise ) );
}

TEST_F( channel, fast_read_two_types )
{
	q::channel< int, std::string > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	std::vector< std::tuple< int, std::string > > expected{
		std::make_tuple( 17, "hello" ), std::make_tuple( 47, "world" )
	};
	std::size_t counter = 0;

	EXPECT_TRUE( writable.write( expected[ 0 ] ) );
	EXPECT_TRUE( writable.write( expected[ 1 ] ) );
	writable.close( );

	auto on_value = [ & ]( int i, std::string s )
	{
		EXPECT_EQ( std::get< 0 >( expected[ counter ] ), i );
		EXPECT_EQ( std::get< 1 >( expected[ counter ] ), s );
		++counter;
	};
	auto on_closed = [ ]( ) { };


	auto promise = readable.read(
		EXPECT_CALL_WRAPPER( on_value ),
		EXPECT_NO_CALL_WRAPPER( on_closed )
	)
	.then( EXPECT_CALL_WRAPPER( [ = ]( bool ) mutable
	{
		return readable.read(
			EXPECT_CALL_WRAPPER( on_value ),
			EXPECT_NO_CALL_WRAPPER( on_closed )
		);
	} ) )
	.then( EXPECT_CALL_WRAPPER( [ = ]( bool ) mutable
	{
		return readable.read(
			EXPECT_NO_CALL_WRAPPER( on_value ),
			EXPECT_CALL_WRAPPER( on_closed )
		);
	} ) );

	run( std::move( promise ) );
}

TEST_F( channel, fast_read_closed_with_exception )
{
	q::channel< int > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	std::vector< int > expected{ 17, 4711 };
	std::size_t counter = 0;

	EXPECT_TRUE( writable.write( expected[ 0 ] ) );
	EXPECT_TRUE( writable.write( expected[ 1 ] ) );
	writable.close( test_exception( ) );

	auto on_value = [ & ]( int i )
	{
		EXPECT_EQ( expected[ counter++ ], i );
	};
	auto on_closed = [ ]( ) { };

	auto promise = readable.read(
		EXPECT_CALL_WRAPPER( on_value ),
		EXPECT_NO_CALL_WRAPPER( on_closed )
	)
	.then( EXPECT_CALL_WRAPPER( [ = ]( bool ) mutable
	{
		return readable.read(
			EXPECT_CALL_WRAPPER( on_value ),
			EXPECT_NO_CALL_WRAPPER( on_closed )
		);
	} ) )
	.then( EXPECT_CALL_WRAPPER( [ = ]( bool ) mutable
	{
		return readable.read(
			EXPECT_NO_CALL_WRAPPER( on_value ),
			EXPECT_NO_CALL_WRAPPER( on_closed )
		);
	} ) )
	.strip( )
	.fail( EXPECT_CALL_WRAPPER( [ ]( test_exception& ) { } ) );

	run( std::move( promise ) );
}

TEST_F( channel, fast_read_exception_when_reading_value )
{
	q::channel< int > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	std::vector< int > expected{ 17, 4711 };
	std::size_t counter = 0;

	EXPECT_TRUE( writable.write( expected[ 0 ] ) );
	EXPECT_TRUE( writable.write( expected[ 1 ] ) );
	writable.close( );

	auto on_value = [ & ]( int i )
	{
		EXPECT_EQ( expected[ counter++ ], i );
		if ( counter == 1 )
			Q_THROW( test_exception( ) );
	};
	auto on_closed = [ ]( ) { };

	auto promise = readable.read(
		EXPECT_CALL_WRAPPER( on_value ),
		EXPECT_NO_CALL_WRAPPER( on_closed )
	)
	.fail( EXPECT_CALL_WRAPPER( [ readable ]( test_exception& )
	{
		EXPECT_TRUE( readable.is_closed( ) );
		return false;
	} ) );

	run( std::move( promise ) );
}

TEST_F( channel, channel_pipe_void )
{
	q::channel< > ch_a( queue, 3 );
	q::channel< > ch_b( queue, 2 );

	auto readable_a = ch_a.get_readable( );
	auto writable_a = ch_a.get_writable( );

	auto readable_b = ch_b.get_readable( );
	auto writable_b = ch_b.get_writable( );

	auto rejected_promise = q::make_promise( queue, [ ]( ) -> int
	{
		Q_THROW( test_exception( ) );
	} ).share( );

	for ( int i = 0; i < 5; ++i )
		q::ignore_result( writable_a.write( ) );

	readable_a.pipe( writable_b );

	for ( int i = 0; i < 5; ++i )
		q::ignore_result( writable_a.write( ) );

	writable_a.close( );

	auto on_close = EXPECT_N_CALLS_WRAPPER( 1, [ ]{ } );

	typedef std::function< q::promise< bool >( ) > fun_type;
	auto read = std::make_shared< fun_type >( );
	*read = EXPECT_N_CALLS_WRAPPER( 11, (
		[ read, readable_b, on_close ]( ) mutable
		{
			return readable_b.read( [ read ]( )
			{
				return ( *read )( ).strip( );
			}, on_close );
		}
	) );

	run( ( *read )( ) );
}

TEST_F( channel, channel_pipe_void_with_error )
{
	q::channel< > ch_a( queue, 3 );
	q::channel< > ch_b( queue, 2 );

	auto readable_a = ch_a.get_readable( );
	auto writable_a = ch_a.get_writable( );

	auto readable_b = ch_b.get_readable( );
	auto writable_b = ch_b.get_writable( );

	auto rejected_promise = q::make_promise( queue, [ ]( ) -> int
	{
		Q_THROW( test_exception( ) );
	} ).share( );

	for ( int i = 0; i < 5; ++i )
		q::ignore_result( writable_a.write( ) );

	readable_a.pipe( writable_b );

	for ( int i = 0; i < 5; ++i )
		q::ignore_result( writable_a.write( ) );

	writable_a.close( Error( ) );

	auto on_close = EXPECT_N_CALLS_WRAPPER( 0, [ ]{ } );

	typedef std::function< q::promise< bool >( ) > fun_type;
	auto read = std::make_shared< fun_type >( );
	*read = EXPECT_N_CALLS_WRAPPER( 11, (
		[ read, readable_b, on_close ]( ) mutable
		{
			return readable_b.read( [ read ]( )
			{
				return ( *read )( ).strip( );
			}, on_close );
		}
	) );

	run(
		( *read )( )
		.strip( )
		.fail( EXPECT_CALL_WRAPPER( [ ]( const Error& ){ } ) )
	);
}

TEST_F( channel, channel_pipe_int )
{
	q::channel< int > ch_a( queue, 3 );
	q::channel< int > ch_b( queue, 2 );

	auto readable_a = ch_a.get_readable( );
	auto writable_a = ch_a.get_writable( );

	auto readable_b = ch_b.get_readable( );
	auto writable_b = ch_b.get_writable( );

	auto rejected_promise = q::make_promise( queue, [ ]( ) -> int
	{
		Q_THROW( test_exception( ) );
	} ).share( );

	for ( int i = 0; i < 5; ++i )
		q::ignore_result( writable_a.write( i ) );

	readable_a.pipe( writable_b );

	for ( int i = 5; i < 10; ++i )
		q::ignore_result( writable_a.write( i ) );

	writable_a.close( );

	int counter = 0;

	auto on_close = EXPECT_N_CALLS_WRAPPER( 1, [ ]{ } );

	typedef std::function< q::promise< bool >( ) > fun_type;
	auto read = std::make_shared< fun_type >( );
	*read = EXPECT_N_CALLS_WRAPPER( 11, (
		[ read, readable_b, &counter, on_close ]( ) mutable
		{
			auto on_data = [ read, &counter ]( int val )
			{
				EXPECT_EQ( val, counter );
				++counter;

				return ( *read )( ).strip( );
			};

			return readable_b.read(
				std::move( on_data ), on_close );
		}
	) );

	run( ( *read )( ) );
}

TEST_F( channel, channel_pipe_int_with_error )
{
	q::channel< int > ch_a( queue, 3 );
	q::channel< int > ch_b( queue, 2 );

	auto readable_a = ch_a.get_readable( );
	auto writable_a = ch_a.get_writable( );

	auto readable_b = ch_b.get_readable( );
	auto writable_b = ch_b.get_writable( );

	auto rejected_promise = q::make_promise( queue, [ ]( ) -> int
	{
		Q_THROW( test_exception( ) );
	} ).share( );

	for ( int i = 0; i < 5; ++i )
		q::ignore_result( writable_a.write( i ) );

	readable_a.pipe( writable_b );

	for ( int i = 5; i < 10; ++i )
		q::ignore_result( writable_a.write( i ) );

	writable_a.close( Error( ) );

	int counter = 0;

	auto on_close = EXPECT_N_CALLS_WRAPPER( 0, [ ]{ } );

	typedef std::function< q::promise< bool >( ) > fun_type;
	auto read = std::make_shared< fun_type >( );
	*read = EXPECT_N_CALLS_WRAPPER( 11, (
		[ read, readable_b, &counter, on_close ]( ) mutable
		{
			auto on_data = [ read, &counter ]( int val )
			{
				EXPECT_EQ( val, counter );
				++counter;

				return ( *read )( ).strip( );
			};

			return readable_b.read( on_data, on_close );
		}
	) );

	run(
		( *read )( )
		.strip( )
		.fail( EXPECT_CALL_WRAPPER( [ ]( const Error& ){ } ) )
	);
}

TEST_F( channel, channel_pipe_async_void )
{
	q::channel< q::promise< > > ch_a( queue, 3 );
	q::channel< q::promise< > > ch_b( queue, 2 );

	auto readable_a = ch_a.get_readable( );
	auto writable_a = ch_a.get_writable( );

	auto readable_b = ch_b.get_readable( );
	auto writable_b = ch_b.get_writable( );

	auto rejected_promise = q::make_promise( queue, [ ]( ) -> int
	{
		Q_THROW( test_exception( ) );
	} ).share( );

	for ( int i = 0; i < 5; ++i )
		q::ignore_result( writable_a.write( ) );

	readable_a.pipe( writable_b );

	for ( int i = 0; i < 5; ++i )
		q::ignore_result( writable_a.write( ) );

	writable_a.close( );

	auto on_close = EXPECT_N_CALLS_WRAPPER( 1, [ ]{ } );

	typedef std::function< q::promise< bool >( ) > fun_type;
	auto read = std::make_shared< fun_type >( );
	*read = EXPECT_N_CALLS_WRAPPER( 11, (
		[ read, readable_b, on_close ]( ) mutable
		-> q::promise< bool >
		{
			return readable_b.read( q::decay_function( [ read ]( )
			{
				return ( *read )( ).strip( );
			} ), on_close );
		}
	) );

	run( ( *read )( ) );
}

TEST_F( channel, channel_pipe_async_void_with_error )
{
	q::channel< q::promise< > > ch_a( queue, 3 );
	q::channel< q::promise< > > ch_b( queue, 2 );

	auto readable_a = ch_a.get_readable( );
	auto writable_a = ch_a.get_writable( );

	auto readable_b = ch_b.get_readable( );
	auto writable_b = ch_b.get_writable( );

	auto rejected_promise = q::make_promise( queue, [ ]( ) -> int
	{
		Q_THROW( test_exception( ) );
	} ).share( );

	for ( int i = 0; i < 5; ++i )
		q::ignore_result( writable_a.write( ) );

	readable_a.pipe( writable_b );

	for ( int i = 0; i < 5; ++i )
		q::ignore_result( writable_a.write( ) );

	writable_a.close( Error( ) );

	auto on_close = EXPECT_N_CALLS_WRAPPER( 0, [ ]{ } );

	typedef std::function< q::promise< bool >( ) > fun_type;
	auto read = std::make_shared< fun_type >( );
	*read = EXPECT_N_CALLS_WRAPPER( 11, (
		[ read, readable_b, on_close ]( ) mutable
		{
			return readable_b.read( [ read ]( )
			{
				return ( *read )( ).strip( );
			}, on_close );
		}
	) );

	run(
		( *read )( )
		.strip( )
		.fail( EXPECT_CALL_WRAPPER( [ ]( const Error& ){ } ) )
	);
}

TEST_F( channel, channel_pipe_async_int )
{
	q::channel< q::promise< int > > ch_a( queue, 3 );
	q::channel< q::promise< int > > ch_b( queue, 2 );

	auto readable_a = ch_a.get_readable( );
	auto writable_a = ch_a.get_writable( );

	auto readable_b = ch_b.get_readable( );
	auto writable_b = ch_b.get_writable( );

	auto rejected_promise = q::make_promise( queue, [ ]( ) -> int
	{
		Q_THROW( test_exception( ) );
	} ).share( );

	for ( int i = 0; i < 5; ++i )
		q::ignore_result( writable_a.write( i ) );

	readable_a.pipe( writable_b );

	for ( int i = 5; i < 10; ++i )
		q::ignore_result( writable_a.write( i ) );

	writable_a.close( );

	int counter = 0;

	auto on_close = EXPECT_N_CALLS_WRAPPER( 1, [ ]{ } );

	typedef std::function< q::promise< bool >( ) > fun_type;
	auto read = std::make_shared< fun_type >( );
	*read = EXPECT_N_CALLS_WRAPPER( 11, (
		[ read, readable_b, &counter, on_close ]( ) mutable
		{
			auto on_data = [ read, &counter ]( int val )
			{
				EXPECT_EQ( val, counter );
				++counter;

				return ( *read )( ).strip( );
			};

			return readable_b.read(
				std::move( on_data ), on_close );
		}
	) );

	run( ( *read )( ) );
}

TEST_F( channel, channel_pipe_async_int_with_error )
{
	q::channel< q::promise< int > > ch_a( queue, 3 );
	q::channel< q::promise< int > > ch_b( queue, 2 );

	auto readable_a = ch_a.get_readable( );
	auto writable_a = ch_a.get_writable( );

	auto readable_b = ch_b.get_readable( );
	auto writable_b = ch_b.get_writable( );

	auto rejected_promise = q::make_promise( queue, [ ]( ) -> int
	{
		Q_THROW( test_exception( ) );
	} ).share( );

	for ( int i = 0; i < 5; ++i )
		q::ignore_result( writable_a.write( i ) );

	readable_a.pipe( writable_b );

	for ( int i = 5; i < 10; ++i )
		q::ignore_result( writable_a.write( i ) );

	writable_a.close( Error( ) );

	int counter = 0;

	auto on_close = EXPECT_N_CALLS_WRAPPER( 0, [ ]{ } );

	typedef std::function< q::promise< bool >( ) > fun_type;
	auto read = std::make_shared< fun_type >( );
	*read = EXPECT_N_CALLS_WRAPPER( 11, (
		[ read, readable_b, &counter, on_close ]( ) mutable
		{
			auto on_data = [ read, &counter ]( int val )
			{
				EXPECT_EQ( val, counter );
				++counter;

				return ( *read )( ).strip( );
			};

			return readable_b.read( on_data, on_close );
		}
	) );

	run(
		( *read )( )
		.strip( )
		.fail( EXPECT_CALL_WRAPPER( [ ]( const Error& ){ } ) )
	);
}

TEST_F( channel, consume_zero_types )
{
	q::channel< > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	EXPECT_TRUE( writable.write( ) );
	EXPECT_TRUE( writable.write( ) );
	writable.close( );

	auto on_value = [ ]( ) { };

	run( readable.consume( EXPECT_N_CALLS_WRAPPER( 2, on_value ) ) );
}

TEST_F( channel, consume_two_types )
{
	q::channel< int, std::string > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	std::vector< std::tuple< int, std::string > > expected{
		std::make_tuple( 17, "hello" ), std::make_tuple( 47, "world" )
	};
	std::size_t counter = 0;

	EXPECT_TRUE( writable.write( expected[ 0 ] ) );
	EXPECT_TRUE( writable.write( expected[ 1 ] ) );
	writable.close( );

	auto on_value = [ & ]( int i, std::string s )
	{
		EXPECT_EQ( std::get< 0 >( expected[ counter ] ), i );
		EXPECT_EQ( std::get< 1 >( expected[ counter ] ), s );
		++counter;
	};

	run( readable.consume( EXPECT_N_CALLS_WRAPPER( 2, on_value ) ) );
}

TEST_F( channel, consume_one_type_concurrent )
{
	q::channel< int > ch( queue, 10 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	std::vector< int > expected{
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
		11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
		21, 22, 23, 24, 25, 26, 27, 28, 29, 30
	};
	std::size_t counter = 0;
	std::atomic< std::size_t > active( 0 );
	std::atomic< std::size_t > active_acc( 0 );

	for ( auto i : expected )
		EXPECT_TRUE( writable.write( i ) );
	writable.close( );

	auto queue = this->queue;

	auto on_value = [ & ]( int i )
	-> q::promise< >
	{
		EXPECT_EQ( expected[ counter ], i );
		++counter;

		++active;
		active_acc += ( active - 1 );

		return q::with( queue )
		.delay( std::chrono::milliseconds( 1 ) )
		.finally( [ &, i ]( )
		{
			--active;
		} );
	};

	run(
		readable
		.consume(
			EXPECT_N_CALLS_WRAPPER( expected.size( ), on_value ),
			{ q::concurrency( 2 ) }
		)
		.then( [ & ]( )
		{
			EXPECT_GT( active_acc, std::size_t( 0 ) );
		} )
	);
}

TEST_F( channel, readable_destruction_should_close )
{
	q::writable< int > writable;

	{
		q::channel< int > ch( queue, 5 );

		auto readable = ch.get_readable( );
		writable = ch.get_writable( );
	}

	EXPECT_FALSE( writable.should_write( ) );
	EXPECT_TRUE( writable.is_closed( ) );
}

TEST_F( channel, writable_destruction_should_close )
{
	q::readable< int > readable;

	{
		q::channel< int > ch( queue, 5 );

		readable = ch.get_readable( );
		auto writable = ch.get_writable( );
	}

	EVENTUALLY_EXPECT_REJECTION_WITH(
		readable.read( ), q::channel_closed_exception );
	EXPECT_TRUE( readable.is_closed( ) );
}
