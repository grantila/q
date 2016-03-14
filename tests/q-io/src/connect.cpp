
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

	auto promise_server = server_socket->clients( )->receive( )
	.then( [ ]( q::io::socket_ptr client )
	{
		std::cout << "GOT client" << std::endl;

		client->in( )->receive( )
		.then( [ ]( q::byte_block&& block )
		{
			std::cout
				<< "GOT client GOT " << block.size( )
				<< " bytes: " << block.to_string( )
				<< std::endl;
		} );
	} );

	auto promise_client = q::with( io_queue )
	.then( io_dispatcher->delay( std::chrono::milliseconds( 10 ) ) )
	.then( [ this ]
	{
		auto dest_ip = q::io::ip_addresses( "127.0.0.1" );
		return io_dispatcher->connect_to( dest_ip, 1030 );
	} )
	.then( [ ]( q::io::socket_ptr socket )
	{
		socket->out( )->send( q::byte_block( "hello world" ) );
		// Socket will go out-of-scope here, i.e. be deleted... WAT?
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
