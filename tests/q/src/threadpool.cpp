
#include <q/execution_context.hpp>
#include <q/blocking_dispatcher.hpp>
#include <q/threadpool.hpp>
#include <q/promise.hpp>

#include <gtest/gtest.h>

#include "q-test.hpp"

TEST( ThreadPool, PerformTasks )
{
	qtest::spy spy;

	auto bd = q::make_execution_context<
		q::blocking_dispatcher, q::direct_scheduler
	>( "test" );
	auto tp = q::threadpool::construct( "test", bd->queue( ) );

	auto task = EXPECT_CALL_WRAPPER( spy )(
		[ tp, &spy ]( )
		{
			auto task = EXPECT_CALL_WRAPPER( spy )(
				[ tp ]( )
				{
					;
				}
			);

			tp->add_task( task );
		}
	);

	tp->add_task( task );

	tp->terminate( q::termination::linger )
	.finally( [ bd ]( )
	{
		bd->dispatcher( )->terminate( q::termination::linger );
	} );

	bd->dispatcher( )->start( );
	tp->await_termination( );
	bd->dispatcher( )->await_termination( );
}
