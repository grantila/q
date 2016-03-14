
#include <q/type_traits.hpp>
#include <q/functional.hpp>
#include <q/log.hpp>
#include <q/promise.hpp>

#include <q/blocking_dispatcher.hpp>
#include <q/threadpool.hpp>
#include <q/scheduler.hpp>
#include <q/stacktrace.hpp>

#include <q/channel.hpp>

#include <q/q.hpp>

#include <q/execution_context.hpp>

#include <qio/dispatcher.hpp>
#include <qio/socket.hpp>
#include <qio/dns.hpp>

#include <string>
#include <iostream>

#include <unistd.h>

q::scope initialize( )
{
	q::settings settings;
	settings.set_long_stack_support( true );

	return q::scoped_initialize( settings );
}

struct my_exception : q::exception { };

class print_line
{
public:
	print_line( std::ostream& out = std::cout )
	: out_( out )
	{ }

	~print_line( )
	{
		ss_ << std::endl;
		out_ << ss_.str( );
	}

	template< typename T >
	print_line& operator<<( T&& t )
	{
		ss_ << t;
		return *this;
	}

private:
	std::stringstream ss_;
	std::ostream& out_;
};

int main( int argc, char** argv )
{
	auto q_scope = initialize( );

	std::cout << "Starting..." << std::endl;

	auto bd = q::make_execution_context<
		q::blocking_dispatcher, q::direct_scheduler
	>( "playground-io" );

	auto tpd = q::make_execution_context<
		q::threadpool, q::direct_scheduler
	>( "threadpool", bd->queue( ) );

	auto dispatcher = q::io::dispatcher::construct( tpd->queue( ) );

	dispatcher->start( );

	q::queue_ptr ev_q = q::queue::construct( 0 );

	auto ec = q::make_execution_context< q::threadpool, q::direct_scheduler >(
		"main background pool", ev_q );

	auto default_queue = ec->queue( );

	std::cout << "Backend: " << dispatcher->backend_method( ) << std::endl;

	q::with( default_queue, 1 )
	.then( dispatcher->delay( std::chrono::milliseconds( 100 ) ) )
	.then( [ dispatcher ]( int )
	{
		print_line( ) << "Events: " << dispatcher->dump_events( );
	} );

	std::cout << "Now doing things..." << std::endl;
	q::with( default_queue, 5 )
	.then( [ ]( int x ) -> int
	{
		print_line( ) << "before x = " << x;
		return x;
	} )
	.then( dispatcher->delay( std::chrono::milliseconds( 500 ) ) )
	.then( [ ]( int x )
	{
		print_line( ) << "after x = " << x;
	} );

	auto dns_resolver = q::make_shared< q::io::resolver >( dispatcher );
	auto time1 = std::chrono::system_clock::now( );
	dns_resolver->lookup(
		default_queue, "www.google.com",
		q::io::resolver::resolve_flags::ipv4 | q::io::resolver::resolve_flags::ipv6 )
	.then( [ time1 ]( q::io::resolver_response&& response )
	{
		auto time2 = std::chrono::system_clock::now( );

		std::cout << response.ips.ipv4.size( ) << " A records:" << std::endl;
		for ( auto& address : response.ips.ipv4 )
			std::cout << "  " << address.string( ) << std::endl;

		std::cout << response.ips.ipv6.size( ) << " AAAA records:" << std::endl;
		for ( auto& address : response.ips.ipv6 )
			std::cout << "  " << address.string( ) << std::endl;

		std::cout << "Took " << std::chrono::duration_cast< std::chrono::milliseconds>( time2 - time1 ).count( ) << " ms" << std::endl;
	} );

	std::string fail_addr = "127.0.0.1";
	std::string succ_addr = "23.235.43.81";

	dispatcher->connect_to( q::io::ip_addresses( fail_addr, succ_addr ), 80 )
	.then( [ ]( q::io::socket_ptr socket )
	{
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
	} )
	.fail( [ ]( const q::io::connection_failed& e )
	{
		std::cout << "CONNECT ERROR [connection failed] " << std::endl;
	} )
	.fail( [ ]( std::exception_ptr e )
	{
		std::cout << "CONNECT ERROR " << q::stream_exception( e ) << std::endl;
	} );

	sleep( 10 );

	dispatcher->terminate( q::io::dispatcher_termination::graceful )
	.then( [ ]( q::io::dispatcher_exit exit )
	{
		typedef std::underlying_type< q::io::dispatcher_exit >::type
			dispatcher_exit_type;
		print_line( )
			<< "Done: "
			<< static_cast< dispatcher_exit_type >( exit );
	} )
	.finally( [ tpd ]( )
	{
		return tpd->dispatcher( )->terminate( q::termination::linger );
	} );

	auto exp = dispatcher->await_termination( );

	if ( exp.has_exception( ) )
		print_line( )
			<< "libevent thread terminated with exception: "
			<< q::stream_exception( exp.exception( ) );
	else
		print_line( )
			<< "libevent thread terminated successfully";

	q::with( bd->queue( ) )
	.then( [ bd ]( )
	{
		bd->dispatcher( )->terminate( q::termination::linger );
	} );

	print_line( ) << "-> A";
	bd->dispatcher( )->start( );
	print_line( ) << "-> B";
	bd->dispatcher( )->await_termination( );
	print_line( ) << "-> C";

	return 0;
}
