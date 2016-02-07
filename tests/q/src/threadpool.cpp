
#include <q/execution_context.hpp>
#include <q/blocking_dispatcher.hpp>
#include <q/threadpool.hpp>
#include <q/promise.hpp>

#include <gtest/gtest.h>

#include <queue>

#include "q-test.hpp"

TEST( ThreadPool, PerformTasks )
{
	qtest::spy spy;

	auto bd = q::make_execution_context<
		q::blocking_dispatcher, q::direct_scheduler
	>( "test" );
	auto tp = q::threadpool::construct( "test", bd->queue( ) );

	auto tasks = q::make_shared< q::queue >( 0 );

	tp->set_task_fetcher( [ tasks ]( )
	{
		return tasks->pop( );
	} );

	auto task = EXPECT_CALL_WRAPPER( spy )(
		[ tasks, tp, &spy ]( )
		{
			auto task = EXPECT_CALL_WRAPPER( spy )(
				[ tp ]( )
				{
					;
				}
			);


			tasks->push( task );
			tp->notify( );
		}
	);

	tasks->push( task );
	tp->notify( );

	tp->terminate( q::termination::linger )
	.finally( [ bd ]( )
	{
		bd->dispatcher( )->terminate( q::termination::linger );
	} );

	bd->dispatcher( )->start( );
	tp->await_termination( );
	bd->dispatcher( )->await_termination( );
}
