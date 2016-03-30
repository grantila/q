
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

#include <q-io/dispatcher.hpp>
#include <q-io/socket.hpp>
#include <q-io/dns.hpp>

#include <qurl/qurl.hpp>
#include <qurl/handle.hpp>
#include <qurl/context.hpp>

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

	auto b_queue = bd->queue( );

	auto tpd = q::make_execution_context<
		q::threadpool, q::direct_scheduler
	>( "threadpool", b_queue );

	auto tp_queue = tpd->queue( );

	auto dispatcher = q::io::dispatcher::construct( tp_queue );

	dispatcher->start( );

	auto default_queue = tp_queue;

	std::cout << "Backend: " << dispatcher->backend_method( ) << std::endl;

	auto curl_ctx = qurl::context::construct( dispatcher );

	auto curl_handle = curl_ctx->create_handle( );

	curl_handle->get_stuff( );

	std::string bbc_host = "www.bbc.com";
	std::string bbc_addr = "23.235.43.81";

	sleep( 2 );

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
