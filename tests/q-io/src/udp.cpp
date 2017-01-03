
#include "q-io-test.hpp"

#include <q-io/dispatcher.hpp>
#include <q-io/udp_receiver.hpp>

QIO_TEST_MAKE_SCOPE( udp );

TEST_F( udp, receive_test_to_be_removed )
{
	auto port = get_next_port( );

	run(
		io_dispatcher->get_udp_receiver( port )
		.then( [ & ]( q::io::udp_receiver_ptr&& receiver )
		{
			auto readable = receiver->get_readable( );
			receiver->detach( );
			std::cout << "X" << std::endl;

			return readable
			.consume( [ & ]( q::io::udp_packet&& packet )
			{
				std::cout
					<< packet.remote_address.string( )
					<< ": "
					<< packet.data.get( ).to_string( )
					<< std::endl;
			} );
		} )
	);
}

TEST_F( udp, send_receive )
{
	auto port = get_next_port( );

	auto receiver = io_dispatcher->udp_receive( port )
	.then( [ & ]( q::readable< q::io::udp_packet >&& r )
	{
		return r.consume( [ r ]( q::io::udp_packet&& packet ) mutable
		{
			EXPECT_EQ( packet.data.get( ).to_string( ), "foobar" );
			r.close( );
		} );
	} );

	auto sender = receiver
	.then( [ & ]( )
	{
		io_dispatcher->udp_send( "127.0.0.1", port )
		.then( [ ]( q::writable< q::byte_block >&& w )
		{
			EXPECT_TRUE( w.send( q::byte_block( "foobar" ) ) );
		} );
	} );

	run( std::move( sender ) );
}
