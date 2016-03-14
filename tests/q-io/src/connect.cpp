
#include "q-io-test.hpp"

#include <q-io/server_socket.hpp>

QIO_TEST_MAKE_SCOPE( connect );

TEST_F( connect, client_server_connrefused )
{
	auto dest = q::io::ip_addresses( "127.0.0.1" );

	auto promise = io_dispatcher->connect_to( dest, 1 )
	.then( [ ]( q::io::socket_ptr socket )
	{
		EXPECT_TRUE( false );
	} )
	.fail( EXPECT_CALL_WRAPPER( spy )(
		[ ]( const q::errno_connrefused_exception& e ) { }
	) )
	.fail( [ ]( std::exception_ptr e )
	{
		EXPECT_TRUE( false );
	} );

	run( std::move( promise ) );
}

TEST_F( connect, client_server2 )
{
	auto server_socket = io_dispatcher->listen( 1030 );

	auto promise_client = server_socket->clients( )->receive( )
	.then( [ ]( q::io::socket_ptr client )
	{
		std::cout << "GOT client" << std::endl;
	} );

	auto promise_server = io_dispatcher->connect_to( q::io::ip_addresses( "127.0.0.1" ), 1030 )
	.then( [ ]( q::io::socket_ptr socket )
	{
	/*
		q::byte_block b( "GET / HTTP/1.1\r\nHost: www.bbc.com\r\n\r\n" );

		std::cout << "SENDING" << std::endl;

		socket->out( )->send( b );

		auto TEMP_COUNTER = std::make_shared< std::atomic< int > >( 0 );

		auto try_get_more = std::make_shared< std::unique_ptr< q::task > >( nullptr );

		*try_get_more = q::make_unique< q::task >( [ TEMP_COUNTER, try_get_more, socket ]( ) mutable
		{
			socket->in( )->receive( )
			.then( [ try_get_more, TEMP_COUNTER ]( q::byte_block&& block )
			{
				std::size_t size = block.size( );

				( *( *try_get_more ) )( );

				TEMP_COUNTER->fetch_add( size );

				std::cout << "GOT " << size << " / " << *TEMP_COUNTER << " b DATA BACK: " << block.to_string( ) << std::endl;
			} )
			.fail( [ try_get_more, TEMP_COUNTER ]( const q::channel_closed_exception& ) mutable
			{
				std::cout << "READING CHANNEL CLOSED: " << *TEMP_COUNTER << std::endl;
				try_get_more.reset( );
			} )
			.fail( [ try_get_more ]( std::exception_ptr e ) mutable
			{
				std::cout << "READING CHANNEL GOT ERROR" << q::stream_exception( e ) << std::endl;
				try_get_more.reset( );
			} );
		} );

		( *( *try_get_more ) )( );
	*/
	} )
	.fail( [ ]( const q::errno_connrefused_exception& e )
	{
		std::cout << "CONNECT ERROR [connection refused] " << std::endl;
	} )
	.fail( [ ]( std::exception_ptr e )
	{
		std::cout << "CONNECT ERROR " << q::stream_exception( e ) << std::endl;
	} );

	run( q::all( std::move( promise_client ), std::move( promise_server ) ) );
}
