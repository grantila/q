
#include "q-io-test.hpp"

#include <q-io/server_socket.hpp>

QIO_TEST_MAKE_SCOPE( connect );

TEST_F( connect, client_server_connrefused )
{
	auto dest = q::io::ip_addresses( "127.0.0.1" );

	EVENTUALLY_EXPECT_REJECTION_WITH(
		io_dispatcher->get_tcp_connection( dest, 1 ),
		q::errno_connrefused_exception
	);
}

TEST_F( connect, client_server_send_data )
{
	const std::string test_data = "hello world";

	auto port = get_next_port( );

	auto promise_server = io_dispatcher->listen( port ).share( );

	auto server_complete = promise_server
	.then( [ test_data ]( q::io::server_socket_ptr socket_server )
	{
		return socket_server->clients( ).read( )
		.then(
		[ socket_server, &test_data ]( q::io::tcp_socket_ptr client )
		{
			return client->in( ).read( )
			.then( [ client, &test_data ]( q::byte_block&& block )
			{
				return block.to_string( );
			} );
		} );
	} )
	.share( );

	EVENTUALLY_EXPECT_EQ( server_complete, test_data );

	auto promise_client = promise_server
	.strip( )
	.use_queue( io_queue )
	.delay( std::chrono::milliseconds( 1 ) )
	.then( [ this, port ]
	{
		auto dest_ip = q::io::ip_addresses( "127.0.0.1" );
		return io_dispatcher->get_tcp_connection( dest_ip, port );
	} )
	.then( [ &test_data ]( q::io::tcp_socket_ptr socket )
	{
		auto writable = socket->out( );
		q::ignore_result(
			writable.write( q::byte_block( test_data ) ) );
		socket->detach( );
	} );

	run( q::all( std::move( promise_client ), server_complete ) );
}

TEST_F( connect, client_server_close_client_on_destruction )
{
	auto port = get_next_port( );

	auto promise_server = io_dispatcher->listen( port )
	.then( [ this ]( q::io::server_socket_ptr socket_server )
	{
		return socket_server->clients( ).read( )
		.then( [ this, socket_server ]( q::io::tcp_socket_ptr client )
		{
			EVENTUALLY_EXPECT_REJECTION_WITH(
				client->in( ).read( ),
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
		return io_dispatcher->get_tcp_connection( dest_ip, port )
		// Get the socket and then let it destruct
		.then( [ ]( q::io::tcp_socket_ptr socket ) { } );
	} );

	run( q::all( std::move( promise_client ), std::move( promise_server ) ) );
}
