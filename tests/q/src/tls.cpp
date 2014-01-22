
#include <q/execution_context.hpp>
#include <q/thread.hpp>
#include <q/tls.hpp>
#include <q/memory.hpp>

#include <gtest/gtest.h>

namespace {

struct tls_test
{
	tls_test( )
	{
		notify_function( 1 );
	}

	~tls_test( )
	{
		if ( notify_function )
			notify_function( -1 );
	}

	int val;

	static std::function< void( int ) > notify_function;
};
std::function< void( int ) > tls_test::notify_function;

} // anonymous namespace

TEST( TLS, AllocDealloc )
{
	auto bd = q::make_shared< q::blocking_dispatcher >( "test dispatcher" );
	auto ctx = q::make_shared< q::execution_context >( bd );
	auto qu = ctx->queue( );

	q::set_default_queue( qu );

	std::atomic< int > counter( 0 );

	tls_test::notify_function = [ &counter ]( int i )
	{
		counter += i;
	};

	q::tls< tls_test >( ).val = 17;

	auto thread1 = q::run( "test", [ ]( ) -> int
	{
		q::tls< tls_test >( ).val = 4711;
		EXPECT_EQ( q::tls< tls_test >( ).val, 4711 );
		return q::tls< tls_test >( ).val;
	} );

	EXPECT_EQ( q::tls< tls_test >( ).val, 17 );

	std::atomic< bool > threaded( false );

	thread1->async_join( )
	.then( [ &threaded, bd ]( int ret )
	{
		EXPECT_EQ( ret, 4711 );
		threaded = true;
		bd->terminate( q::termination::linger );
	}, qu );

	bd->start( );

	tls_test::notify_function = nullptr;
}
