
#include <q/promise.hpp>
#include <q/lib.hpp>
#include <q/blocking_dispatcher.hpp>
#include <q/scheduler.hpp>
#include <q/timer.hpp>

#include <iomanip>

struct Copyable
{
	Copyable( ) = default;
	Copyable( const Copyable& ) = default;
	Copyable& operator=( const Copyable& ) = default;

	Copyable( Copyable&& ) = delete;
	Copyable& operator=( Copyable&& ) = delete;
};

struct Movable
{
	Movable( ) = default;
	Movable( Movable&& ) = default;
	Movable& operator=( Movable&& ) = default;

	Movable( const Movable& ) = delete;
	Movable& operator=( const Movable& ) = delete;
};

void print_timer( std::ostream& os, q::timer::duration_type dur, std::size_t denominator )
{
	auto ns = std::chrono::duration_cast< std::chrono::nanoseconds >( dur );
	auto float_ns = static_cast< double >( ns.count( ) );
	double partdur = float_ns / denominator;

	os
		<< std::fixed
		<< std::setprecision( 4 )
		<< partdur / 1000
		<< "us (i.e. "
		<< std::setprecision( 0 )
		<< ( 1000.0 * 1000.0 * 1000.0 ) / partdur
		<< "/s)"
		<< std::endl;
}

int main( int argc, char** argv )
{
	q::settings settings;
	settings.set_long_stack_support( true );

	auto scope = q::scoped_initialize( settings );

	auto bd = q::make_shared< q::blocking_dispatcher >( "main" );
	auto queue = q::make_shared< q::queue >( );
	q::set_main_queue( queue );
	q::set_default_queue( queue );
	auto sched = q::make_shared< q::scheduler >( bd );
	sched->add_queue( queue );



	q::promise< std::tuple< int > > promise = q::with( 0 );

	std::size_t maxnum = 1000 * 1000;

	{
		q::scoped_timer::function_type fn = [ maxnum ]( q::timer::duration_type dur )
		{
			std::cout << "Queueing a task takes ";
			print_timer( std::cout, dur, maxnum );
		};
		q::scoped_timer timer( std::move( fn ) );

		for ( std::size_t i = 0; i < maxnum; ++i )
			promise = promise.then( [ ]( int value )
			{
				return value + 1;
			} );
	}

	std::size_t final;
	promise.then( [ &final, bd ]( int value )
	{
		final = value;
		bd->terminate( q::event_dispatcher::termination::linger );
	} );

	{
		q::scoped_timer::function_type fn = [ maxnum ]( q::timer::duration_type dur )
		{
			std::cout << "Scheduling and running a task takes ";
			print_timer( std::cout, dur, maxnum );
		};

		q::scoped_timer timer( std::move( fn ) );
		
		bd->start( );
	}

}
