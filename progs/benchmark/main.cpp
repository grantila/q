
#include <q/promise.hpp>
#include <q/lib.hpp>
#include <q/blocking_dispatcher.hpp>
#include <q/threadpool.hpp>
#include <q/execution_context.hpp>
#include <q/scheduler.hpp>
#include <q/timer.hpp>

#include <iomanip>
#include <numeric>

void print_timer( std::ostream& os, q::timer::duration_type dur, std::size_t denominator )
{
	auto ns = std::chrono::duration_cast< std::chrono::nanoseconds >( dur );
	auto float_ns = static_cast< double >( ns.count( ) );
	double partdur = float_ns / denominator;

	os
		<< std::fixed << std::setprecision( 0 )
		<< partdur << "ns (i.e. "
		<< ( 1000.0 * 1000.0 * 1000.0 ) / partdur << "/s)"
		<< std::endl;
}

void benchmark_title( const std::string& title )
{
	std::string msg = "Benchmarking: " + title;
	std::string lines( msg.size( ), '=' );

	std::cout << lines << std::endl;
	std::cout << msg << std::endl;
	std::cout << lines << std::endl;
}

template< typename Start, typename Stop >
void benchmark_queueing_and_scheduling( Start&& start, Stop&& stop, bool parallel, q::queue_ptr queue, std::size_t iterations )
{
	q::shared_promise< std::tuple< int > > promise = q::with( 0 ).share( );
	std::vector< q::promise< std::tuple< int > > > waitable;
	q::promise< std::tuple< int > > serial_promise = q::with( 0 );
	waitable.reserve( iterations );

	q::timer::duration_type total_dur( 0 );

	{
		q::scoped_timer::function_type fn = [ &total_dur, iterations ]( q::timer::duration_type dur )
		{
			total_dur += dur;

			std::cout << "Queueing a task takes ";
			print_timer( std::cout, dur, iterations );
		};
		q::scoped_timer timer( std::move( fn ) );

		if ( parallel )
			for ( std::size_t i = 0; i < iterations; ++i )
				waitable.push_back(
					promise.then( [ ]( int value )
					{
						return value + 1;
					}, queue )
				);
		else
			for ( std::size_t i = 0; i < iterations; ++i )
				serial_promise = serial_promise
				.then( [ ]( int value )
				{
					return value + 1;
				}, queue );
	}

	std::size_t final = 0;
	if ( parallel )
	{
		q::scoped_timer::function_type fn = [ &total_dur, iterations ]( q::timer::duration_type dur )
		{
			total_dur += dur;

			std::cout << "Scheduling completions for " << iterations << " tasks using q::all() takes per task ";
			print_timer( std::cout, dur, iterations );
		};

		q::scoped_timer timer( std::move( fn ) );

		q::all( std::move( waitable ) )
		.then( [ &final, stop ]( std::vector< int >&& values )
		{
			int sum = std::accumulate( values.begin( ), values.end( ), 0 );
			final += sum;
			stop( );
		} );
	}
	else
	{
		serial_promise
		.then( [ &final, stop ]( int val )
		{
			final = val;
			stop( );
		} );
	}

	{
		q::scoped_timer::function_type fn = [ &total_dur, iterations ]( q::timer::duration_type dur )
		{
			total_dur += dur;

			std::cout << "Scheduling and running a task takes ";
			print_timer( std::cout, dur, iterations );
		};

		q::scoped_timer timer( std::move( fn ) );

		start( );
	}

	if ( final != iterations )
	{
		std::cerr
			<< "Error running benchmark, wrong amount iterations: "
			<< final << " != " << iterations << "!" << std::endl;
		return;
	}

	std::cout << "Queueing, scheduling and running takes ";
	print_timer( std::cout, total_dur, iterations );
}

void benchmark_tasks_on_main_queue( std::size_t iterations, bool parallel )
{
	auto bd2 = q::make_shared< q::blocking_dispatcher >( "test" );
	auto ctx2 = q::make_shared< q::execution_context >( bd2 );
	auto queue = ctx2->queue( );
	auto prev_queue = q::set_default_queue( queue );

	auto start = [ bd2 ]( )
	{
		bd2->start( );
	};
	auto stop = [ bd2 ]( )
	{
		bd2->terminate( q::termination::linger );
	};

	std::string in_parallel;
	if ( parallel )
		in_parallel = " in parallel";

	benchmark_title( "queueing, scheduling and running trivial tasks" + in_parallel );
	benchmark_queueing_and_scheduling( start, stop, parallel, queue, iterations );
	std::cout << std::endl;

	q::set_default_queue( prev_queue );
}

void benchmark_tasks_on_threadpool( std::size_t iterations, bool parallel )
{
	auto bd2 = q::make_shared< q::blocking_dispatcher >( "test" );
	auto ctx2 = q::make_shared< q::execution_context >( bd2 );
	auto queue = ctx2->queue( );
	auto prev_queue = q::set_default_queue( queue );

	auto tp = q::make_shared< q::threadpool >( "threadpool" );
	auto bg_queue = q::make_shared< q::queue >( );
	q::set_background_queue( bg_queue );
	auto bg_sched = q::make_shared< q::scheduler >( tp );

	auto start = [ bd2, bg_sched, bg_queue ]( )
	{
		bg_sched->add_queue( bg_queue );
		bd2->start( );
	};
	auto stop = [ bd2, tp ]( )
	{
		return bd2->terminate( q::termination::linger )
		.then( [ tp ]( )
		{
			return tp->terminate( );
		});
	};

	std::string in_parallel;
	if ( parallel )
		in_parallel = " in parallel";

	benchmark_title( "queueing, scheduling and running trivial tasks" + in_parallel + " on thread pool" );
	benchmark_queueing_and_scheduling( start, stop, parallel, bg_queue, iterations );
	std::cout << std::endl;

	q::set_default_queue( prev_queue );
}

int main( int argc, char** argv )
{
	q::settings settings;
	settings.set_long_stack_support( true );

	auto scope = q::scoped_initialize( settings );

	auto bd = q::make_shared< q::blocking_dispatcher >( "main" );
	auto ctx = q::make_shared< q::execution_context >( bd );
	auto qu = ctx->queue( );

	q::set_main_queue( qu );
	q::set_default_queue( qu );

	std::size_t iterations = 500 * 1000;

	benchmark_tasks_on_main_queue( iterations, true );
	benchmark_tasks_on_threadpool( iterations, true );

	benchmark_tasks_on_main_queue( iterations, false );
	benchmark_tasks_on_threadpool( iterations, false );
}
