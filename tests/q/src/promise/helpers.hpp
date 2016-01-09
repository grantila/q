/*
 * Copyright 2015 Gustaf Räntilä
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBQ_TEST_PROMISE_HELPERS_HPP
#define LIBQ_TEST_PROMISE_HELPERS_HPP

#include <q/promise.hpp>
#include <q/execution_context.hpp>
#include <q/threadpool.hpp>
#include <q/scope.hpp>

#include <gtest/gtest.h>

#include "../q-test.hpp"


class PromiseSetup
: public ::testing::Test
{
public:
	PromiseSetup( )
	: scope_( nullptr )
	{ }

	class Error
	: public ::q::exception
	{ };

protected:
	virtual void SetUp( )
	{
		bd = q::make_execution_context< q::blocking_dispatcher >( "all" );
		queue = bd->queue( );
	}

	virtual void TearDown( )
	{
		scope_ = std::move( q::make_scope( nullptr ) );
		bd->dispatcher( )->await_termination( );
		bd.reset( );
	}

	template< typename Promise >
	void run( Promise&& promise )
	{
		promise
		.finally( [ this ]( )
		{
			bd->dispatcher( )->terminate( q::termination::linger );
		} );

		bd->dispatcher( )->start( );
	}

	std::shared_ptr< q::specific_execution_context< q::blocking_dispatcher > > bd;
	std::shared_ptr< q::specific_execution_context< q::threadpool > > tp;

	q::queue_ptr queue;
	q::queue_ptr tp_queue;

	qtest::spy spy;

private:
	q::scope scope_;
};

#define Q_TEST_MAKE_SCOPE( name ) \
	class name : public PromiseSetup { }

#endif // LIBQ_TEST_PROMISE_HELPERS_HPP
