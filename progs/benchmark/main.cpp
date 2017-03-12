
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

q::scoped_timer make_benchmark_timer(
	q::timer::duration_type& total_dur,
	std::size_t iterations,
	std::string title
)
{
	auto fn = [ &total_dur, iterations, title ]( q::timer::duration_type dur )
	{
		total_dur += dur;

		std::cout << title << ": ";
		print_timer( std::cout, dur, iterations );
	};

	return q::scoped_timer( std::move( fn ) );
}

static inline void empty_fn( ) { }

#ifdef LIBQ_ON_WINDOWS
#	define BRACE_INIT( value ) { value }
#else
#	define BRACE_INIT( value )
#endif

template< template< typename > class Fn >
void benchmark_q_function( std::size_t iterations, std::string impl )
{
	benchmark_title( impl );

	q::timer::duration_type total_dur( 0 );

	{
		auto timer_scope = make_benchmark_timer(
			total_dur, iterations, "void()" );

		for ( std::size_t i = 0; i < iterations; ++i )
		{
			Fn< void( ) > fn( empty_fn );
			fn( );
		}
	}

	{
		auto timer_scope = make_benchmark_timer(
			total_dur, iterations, "Non-capturing lambda< void() >" );

		auto l = [ ]( ) { };

		for ( std::size_t i = 0; i < iterations; ++i )
		{
			Fn< void( ) > fn( l );
			fn( );
		}
	}

	{
		auto timer_scope = make_benchmark_timer(
			total_dur, iterations, "Small (16 bytes) capturing lambda< void() >" );

		typedef char cap_type[ 16 ];

		cap_type cap;
		auto l = [ cap BRACE_INIT( cap ) ]( ) { };

		for ( std::size_t i = 0; i < iterations; ++i )
		{
			Fn< void( ) > fn( l );
			fn( );
		}
	}

	{
		auto timer_scope = make_benchmark_timer(
			total_dur, iterations, "Medium size (64 bytes) capturing lambda< void() >" );

		typedef char cap_type[ 64 ];

		cap_type cap;
		auto l = [ cap BRACE_INIT( cap ) ]( ) { };

		for ( std::size_t i = 0; i < iterations; ++i )
		{
			Fn< void( ) > fn( l );
			fn( );
		}
	}

	{
		auto timer_scope = make_benchmark_timer(
			total_dur, iterations, "Large size (128 bytes) capturing lambda< void() >" );

		typedef char cap_type[ 128 ];

		int i = 0;

		cap_type cap;
		auto l = [ cap BRACE_INIT( cap ), &i ]( ) {
			++i;
		};

		for ( std::size_t i = 0; i < iterations; ++i )
		{
			Fn< void( ) > fn( l );
			fn( );
		}
	}

	{
		auto timer_scope = make_benchmark_timer(
			total_dur, iterations, "Large size (128 bytes) capturing lambda< void() >, copied twice" );

		typedef char cap_type[ 128 ];

		int i = 0;

		cap_type cap;
		auto l = [ cap BRACE_INIT( cap ), &i ]( ) {
			++i;
		};

		for ( std::size_t i = 0; i < iterations; ++i )
		{
			Fn< void( ) > fn( l );
			Fn< void( ) > fn2 = fn;
			Fn< void( ) > fn3 = fn2;
			fn3( );
		}
	}

	std::cout << std::endl;
}

template< typename Start, typename Stop >
void benchmark_queueing_and_scheduling(
	Start&& start,
	Stop&& stop,
	bool parallel,
	q::queue_ptr queue,
	q::queue_ptr main_queue,
	std::size_t iterations
)
{
	q::shared_promise< int > promise = q::with( queue, 0 ).share( );
	std::vector< q::promise< int > > waitable;
	q::promise< int > serial_promise = q::with( queue, 0 );
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

		q::all( std::move( waitable ), queue )
		.then( [ &final, stop ]( std::vector< int >&& values )
		{
			int sum = std::accumulate( values.begin( ), values.end( ), 0 );
			final += sum;
			stop( );
		}, main_queue );
	}
	else
	{
		serial_promise
		.then( [ &final, stop ]( int val )
		{
			final = val;
			stop( );
		}, main_queue );
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
	auto s2 = q::make_shared< q::direct_scheduler >( bd2 );
	auto ctx2 = q::make_shared< q::execution_context >( bd2, s2 );
	auto queue = ctx2->queue( );

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
	benchmark_queueing_and_scheduling( start, stop, parallel, queue, queue, iterations );
	std::cout << std::endl;
}

void benchmark_tasks_on_threadpool( std::size_t iterations, bool parallel )
{
	auto bd2 = q::make_shared< q::blocking_dispatcher >( "test" );
	auto s2 = q::make_shared< q::direct_scheduler >( bd2 );
	auto ctx2 = q::make_shared< q::execution_context >( bd2, s2 );
	auto queue = ctx2->queue( );

	auto tp = q::make_shared< q::threadpool >( "threadpool", queue );
	auto bg_queue = q::make_shared< q::queue >( 0 );
	auto bg_sched = q::make_shared< q::direct_scheduler >( tp );

	auto start = [ tp, bd2, bg_sched, bg_queue ]( )
	{
		bg_sched->add_queue( bg_queue );
		tp->start( );
		bd2->start( );
	};
	auto stop = [ bd2, tp ]( )
	{
		bd2->terminate( q::termination::linger );
		tp->terminate( q::termination::linger );
		tp->await_termination( );
	};

	std::string in_parallel;
	if ( parallel )
		in_parallel = " in parallel";

	benchmark_title( "queueing, scheduling and running trivial tasks" + in_parallel + " on thread pool" );
	benchmark_queueing_and_scheduling( start, stop, parallel, bg_queue, queue, iterations );
	bd2->await_termination( );
	std::cout << std::endl;
}

int main( int, char** )
{
	q::settings settings;
	settings.set_long_stack_support( true );

	auto scope = q::scoped_initialize( settings );

	std::size_t iterations = 500 * 1000;

	auto fn_iterations = iterations * 10;

	benchmark_q_function< std::function >( fn_iterations, "std::function" );
	benchmark_q_function< q::function >( fn_iterations, "q::function" );

	benchmark_tasks_on_main_queue( iterations, true );
	benchmark_tasks_on_threadpool( iterations, true );

	benchmark_tasks_on_main_queue( iterations, false );
	benchmark_tasks_on_threadpool( iterations, false );
}
