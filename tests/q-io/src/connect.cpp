
#include "q-io-test.hpp"

#include <q-io/server_socket.hpp>

QIO_TEST_MAKE_SCOPE( connect );

std::atomic< std::uint16_t > ports( 1030 );

TEST_F( connect, client_server_connrefused )
{
	auto dest = q::io::ip_addresses( "127.0.0.1" );

	EVENTUALLY_EXPECT_REJECTION_WITH(
		io_dispatcher->connect_to( dest, 1 ),
		q::errno_connrefused_exception
	);
}

TEST_F( connect, DISABLED_client_server_send_data )
{
	const std::string test_data = "hello world";

	auto port = ports++;

	auto promise_server = io_dispatcher->listen( port )
	.then( [ test_data ]( q::io::server_socket_ptr socket_server )
	{
		return socket_server->clients( ).receive( )
		.then( [ socket_server, &test_data ]( q::io::socket_ptr client )
		{
			return client->in( ).receive( )
			.then( [ client, &test_data ]( q::byte_block&& block )
			{
				return block.to_string( );
			} );
		} );
	} )
	.share( );

	EVENTUALLY_EXPECT_EQ( promise_server, test_data );

	auto promise_client = q::with( io_queue )
	.then( io_dispatcher->delay( std::chrono::milliseconds( 10 ) ) )
	.then( [ this, port ]
	{
		auto dest_ip = q::io::ip_addresses( "127.0.0.1" );
		return io_dispatcher->connect_to( dest_ip, port );
	} )
	.then( [ &test_data ]( q::io::socket_ptr socket )
	{
		socket->out( ).send( q::byte_block( test_data ) );
		// TODO: Fix this, this sometimes causes failure. Removing the
		// detach() call causes the same error every time.
		socket->detach( );
	} );

	run( q::all( std::move( promise_client ), std::move( promise_server ) ) );
}

TEST_F( connect, client_server_close_client_on_destruction )
{
	auto port = ports++;

	auto promise_server = io_dispatcher->listen( port )
	.then( [ this ]( q::io::server_socket_ptr socket_server )
	{
		return socket_server->clients( ).receive( )
		.then( [ this, socket_server ]( q::io::socket_ptr client )
		{
			EVENTUALLY_EXPECT_REJECTION_WITH(
				client->in( ).receive( ),
				q::channel_closed_exception
			);

			client->detach( );
		} );
	} );

	auto promise_client = q::with( io_queue )
	.then( io_dispatcher->delay( std::chrono::milliseconds( 10 ) ) )
	.then( [ this, port ]
	{
		auto dest_ip = q::io::ip_addresses( "127.0.0.1" );
		return io_dispatcher->connect_to( dest_ip, port )
		// Get the socket and then let it destruct
		.then( [ ]( q::io::socket_ptr socket ) { } );
	} );

	run( q::all( std::move( promise_client ), std::move( promise_server ) ) );
}
