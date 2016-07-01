
#include <q-rx/functional.hpp>
#include <q-rx/observable.hpp>

#include <q/promise.hpp>
#include <q/blocking_dispatcher.hpp>
#include <q/threadpool.hpp>
#include <q/scheduler.hpp>
#include <q/stacktrace.hpp>

#include <q/channel.hpp>

#include <q/q.hpp>

#include <string>
#include <iostream>

Q_MAKE_SIMPLE_EXCEPTION( playground_error );

q::scope initialize( )
{
	q::settings settings;
	settings.set_long_stack_support( true );

	return q::scoped_initialize( settings );
}

struct my_exception : q::exception { };

int main( int argc, char** argv )
{
/*
	q::rx::observable< int > obs;
	obs // o< o< int > >
		.map( q::rx::f::accumulate )
		.map( q::rx::f::div( std::size_t( 3 ) ) )
		.op( q::rx::f::adjacent_difference )
		.zip( obs2, inner_product )
*/

	auto dividor = q::rx::f::div< int >( 5 );

	auto three = dividor( 15 );

	std::cout << "3 = " << three << std::endl;




	auto q_scope = initialize( );

	auto bd = q::make_shared< q::blocking_dispatcher >( "main" );
	auto queue = q::make_shared< q::queue >( 0 );

	auto sched = q::make_shared< q::direct_scheduler >( bd );
	sched->add_queue( queue );



	auto o_empty = q::rx::observable< int >::empty( queue );

	auto o_never = q::rx::observable< int >::never( queue );

	std::vector< int > vec_int{ 1, 2, 3 };

	//auto o_from_vector = q::rx::observable< int >::from( vec_int, queue );
	auto o_from_vector = q::rx::with( queue, vec_int );

	auto consumer = [ queue ]( int i )
	{
		return q::make_promise_sync( queue, [ i ]( )
		{
			std::cout << "GOT i = " << i << std::endl;
		} )
		.then( [ i ]( )
		{
			std::cout << "  -- just a reminder: " << i << std::endl;
		} );
	};

	auto consumption_complete = [ ]( )
	{
		std::cout << "Stream ended" << std::endl;
	};
	auto consumption_failed = [ ]( std::exception_ptr e )
	{
		std::cout << "Stream error " << q::stream_exception( e ) << std::endl;
	};

	o_from_vector
	/*
		.map( [ queue ]( int i )
		{
			std::cout << "Input = " << i << std::endl;
			return i;
		} )
		*/
		.map( q::rx::f::mul( 2 ) )
		.map( [ queue ]( int i )
		{
			return i * 2;//q::with( queue, i * 2 );
		} )
		.consume( consumer )
		.then( consumption_complete )
		.fail( consumption_failed )
		.finally( [ bd ]( )
		{
			bd->terminate( q::termination::linger );
		} );

	bd->start( );

	std::cout << "Sutting down successfully" << std::endl;

	return 0;

/*
	auto observable_bytes = q::rx::from( ch_incoming_byte_blocks );

	const auto dur_window = std::chrono::milliseconds( 100 );
	const auto dur_smooth = std::chrono::seconds( 3 );
	const auto mul_smooth = dur_smooth / std::chrono::seconds( 1 );
	const auto backlog = std:size_t( dur_smooth / dur_window );

	observable_bytes // Sporadic observable< q::byte_block >
		.map( [ ]( q::byte_block block )
		{
			return block.size( );
		} )
		// Sporadic observable< std::size_t > (whenever data arrives)
		.window( dur_window )
		// -> observable< observable< std::size_t > > emitting every 100ms
		.map( [ ]( std::observable< std::size_t > window_sizes )
		{
			return window_sizes.reduce( [ ](
				std::size_t prev, std::size_t cur )
			{
				return prev + cur;
			} );
		} )
		// -> observable< std::size_t > emitting bytes per 100ms (every 100ms)
		.scan( [ ]( std::list< std::size_t > prev, std::size_t cur )
			if ( prev.size( ) > backlog )
				prev.pop_front( );
			prev.push_back( cur );
			return prev;
		} )
		// -> observable< std::list< std::size_t > > every 100ms
		.map( [ ]( std::list< std::size_t > last )
		{
			return std::accumulate(
				last.begin( ), last.end( ), 0 ) / mul_smooth;
		} );
		// -> observable< std::size_t > for bytes per second, every 100ms
*/
}
