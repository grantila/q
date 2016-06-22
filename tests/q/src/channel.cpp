
#include <q/channel.hpp>

#include <q-test/q-test.hpp>
#include <q-test/expect.hpp>

Q_TEST_MAKE_SCOPE( channel );

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

TEST_F( channel, promise_channel )
{
	q::channel< q::promise< std::tuple< int > > > ch( queue, 5 );

	auto readable = ch.get_readable( );
	auto writable = ch.get_writable( );

	EVENTUALLY_EXPECT_EQ( readable.receive( ), 5 );
	EVENTUALLY_EXPECT_EQ( readable.receive( ), 6 );
	EVENTUALLY_EXPECT_REJECTION_WITH(
		readable.receive( ), q::channel_closed_exception );

	writable.send( q::with( queue, 5 ) );
	writable.send( 6 );
	writable.close( );
}
