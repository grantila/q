
#include <q/channel.hpp>

#include <q-test/q-test.hpp>

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

	writable.send( );
	writable.send( std::make_tuple( ) );
	writable.send( q::void_t( ) );
	writable.send( std::make_tuple( q::void_t( ) ) );
	writable.close( );

	auto promise = readable.receive( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( )
		{
			return readable.receive( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( std::tuple< >&& )
		{
			return readable.receive( );
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
			return readable.receive( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( std::tuple< q::void_t >&& )
		{
			return readable.receive( );
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

TEST_F( channel, one_type )
{
	q::channel< int > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	writable.send( 17 );
	writable.send( std::make_tuple< int >( 4711 ) );
	writable.close( );

	auto promise = readable.receive( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 17 );

			return readable.receive( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 4711 );

			return readable.receive( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( int value ) { }
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

	writable.send( 17, "hello" );
	writable.send( std::make_tuple( 4711, "world" ) );
	writable.close( );

	auto promise = readable.receive( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value, std::string&& s )
		{
			EXPECT_EQ( 17, value );
			EXPECT_EQ( "hello", s );

			return readable.receive( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value, std::string s )
		{
			EXPECT_EQ( 4711, value );
			EXPECT_EQ( "world", s );

			return readable.receive( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( int value, std::string&& ) { }
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

		writable.send( 17 );
		writable.send( 4711 );

		return readable;
	};

	auto readable = channel_creator( );

	auto promise = readable.receive( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 17 );

			return readable.receive( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 4711 );

			return readable.receive( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( int value ) { }
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
	-> std::tuple< q::promise< std::tuple< > >, q::writable< int > >
	{
		q::channel< int > ch( queue, 5 );

		auto readable = ch.get_readable( );
		auto writable = ch.get_writable( );

		auto promise = readable.receive( )
		.then( EXPECT_NO_CALL_WRAPPER(
			[ ]( int value ) { }
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

	EXPECT_THROW( writable.send( 17 ), q::channel_closed_exception );

	run( std::move( promise ) );
}

TEST_F( channel, channel_empty_promise_specialization )
{
	typedef q::promise< std::tuple< > > promise_type;

	q::channel< promise_type > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	writable.send( );
	writable.send( std::make_tuple< >( ) );
	writable.send( q::void_t( ) );
	writable.send( std::make_tuple( q::void_t( ) ) );
	writable.close( );

	auto receiver = EXPECT_N_CALLS_WRAPPER( 4,
		[ &readable ]( )
		{
			return readable.receive( );
		}
	);

	auto promise = readable.receive( )
	.then( receiver )
	.then( receiver )
	.then( receiver )
	.then( receiver )
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
	typedef q::promise< std::tuple< int > > promise_type;

	q::channel< promise_type > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	writable.send( 17 );
	writable.send( std::make_tuple< int >( 4711 ) );
	writable.close( );

	auto promise = readable.receive( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 17 );

			return readable.receive( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 4711 );

			return readable.receive( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( int value ) { }
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
	typedef q::promise< std::tuple< int > > promise_type;

	q::channel< promise_type > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	auto rejected_promise = q::make_promise( queue, [ ]( ) -> int
	{
		Q_THROW( test_exception( ) );
	} );

	writable.send( 5 );
	writable.send( std::move( rejected_promise ) );
	writable.send( 17 );
	writable.close( );

	auto promise = readable.receive( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 5 );

			return readable.receive( );
		}
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ &readable ]( const test_exception& e )
		{
			return readable.receive( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ this ]( int value )
		{
			return q::with( queue, 5 );
		}
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ &readable ]( const test_exception& e )
		{
			return readable.receive( );
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
	typedef q::shared_promise< std::tuple< > > promise_type;

	q::channel< promise_type > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	writable.send( );
	writable.send( std::make_tuple< >( ) );
	writable.close( );

	auto promise = readable.receive( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( )
		{
			return readable.receive( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( )
		{
			return readable.receive( );
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
	typedef q::shared_promise< std::tuple< int > > promise_type;

	q::channel< promise_type > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	writable.send( 17 );
	writable.send( std::make_tuple< int >( 4711 ) );
	writable.close( );

	auto promise = readable.receive( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 17 );

			return readable.receive( );
		}
	) )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 4711 );

			return readable.receive( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ ]( int value ) { }
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
	typedef q::shared_promise< std::tuple< int > > promise_type;

	q::channel< promise_type > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	auto rejected_promise = q::make_promise( queue, [ ]( ) -> int
	{
		Q_THROW( test_exception( ) );
	} ).share( );

	writable.send( 5 );
	writable.send( std::move( rejected_promise ) );
	writable.send( 17 );
	writable.close( );

	auto promise = readable.receive( )
	.then( EXPECT_CALL_WRAPPER(
		[ &readable ]( int value )
		{
			EXPECT_EQ( value, 5 );

			return readable.receive( );
		}
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ &readable ]( const test_exception& e )
		{
			return readable.receive( );
		}
	) )
	.then( EXPECT_NO_CALL_WRAPPER(
		[ this ]( int value )
		{
			return q::with( queue, 5 );
		}
	) )
	.fail( EXPECT_CALL_WRAPPER(
		[ &readable ]( const test_exception& e )
		{
			return readable.receive( );
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
