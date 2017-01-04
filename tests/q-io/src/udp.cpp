
#include "q-io-test.hpp"

#include <q-io/dispatcher.hpp>
#include <q-io/udp_receiver.hpp>

QIO_TEST_MAKE_SCOPE( udp );

TEST_F( udp, send_receive )
{
	auto port = get_next_port( );

	std::vector< std::string > messages{
		"test1", "test2", "test3", "test4", "test5"
	};
	std::size_t index = 0;

	auto receiver = io_dispatcher->udp_receive( port )
	.then( [ & ]( q::readable< q::io::udp_packet >&& r )
	{
		auto completion =
		r.consume( [ &, r ]( q::io::udp_packet&& packet ) mutable
		{
			EXPECT_EQ(
				packet.data.get( ).to_string( ),
				messages[ index++ ] );
			if ( index > 3 )
				r.close( );
		} );
		EVENTUALLY_EXPECT_RESOLUTION( std::move( completion ) );
	} );

	auto sender = receiver
	.then( [ & ]( )
	{
		return io_dispatcher->udp_send( "127.0.0.1", port )
		.then( [ & ]( q::writable< q::byte_block >&& w )
		{
			EXPECT_TRUE( w.write( q::byte_block( "test1" ) ) );
			EXPECT_TRUE( w.write( q::byte_block( "test2" ) ) );
			EXPECT_TRUE( w.write( q::byte_block( "test3" ) ) );
			EXPECT_TRUE( w.write( q::byte_block( "test4" ) ) );
			EXPECT_TRUE( w.write( q::byte_block( "test5" ) ) );
		} );
	} );

	run( std::move( sender ) );
}
