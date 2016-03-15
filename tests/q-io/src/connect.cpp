
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

TEST_F( connect, client_server_send_data )
{
	const std::string test_data = "hello world";

	auto socket_server = io_dispatcher->listen( 1030 );

	auto promise_server = socket_server->clients( )->receive( )
	.then( [ socket_server, &test_data ]( q::io::socket_ptr client )
	{
		return client->in( )->receive( )
		.then( [ &test_data ]( q::byte_block&& block )
		{
			EXPECT_EQ( block.to_string( ), test_data );
		} );
	} );

	auto promise_client = q::with( io_queue )
	.then( io_dispatcher->delay( std::chrono::milliseconds( 10 ) ) )
	.then( [ this ]
	{
		auto dest_ip = q::io::ip_addresses( "127.0.0.1" );
		return io_dispatcher->connect_to( dest_ip, 1030 );
	} )
	.then( [ &test_data ]( q::io::socket_ptr socket )
	{
		socket->out( )->send( q::byte_block( test_data ) );
		socket->detach( );
	} );

	run( q::all( std::move( promise_client ), std::move( promise_server ) ) );
}
