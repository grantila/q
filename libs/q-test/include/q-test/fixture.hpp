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

#ifndef LIBQ_TEST_FIXTURE_HPP
#define LIBQ_TEST_FIXTURE_HPP

#include <q-test/gtest.hpp>
#include <q-test/spy.hpp>

#include <q/promise.hpp>
#include <q/execution_context.hpp>
#include <q/threadpool.hpp>
#include <q/scope.hpp>

#define Q_TEST_MAKE_SCOPE( name ) \
	class name : public ::q::test::fixture { }

namespace q { namespace test {

class fixture
: public ::testing::Test
{
public:
	fixture( );
	virtual ~fixture( );

	Q_MAKE_SIMPLE_EXCEPTION( Error );

	void await_promise( ::q::promise< std::tuple< > >&& promise )
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		awaiting_promises_.emplace_back( std::move( promise ) );
	}

protected:
	virtual void on_setup( ) { }
	virtual void on_teardown( ) { }

	void SetUp( ) override;
	void TearDown( ) override;

	void keep_alive( q::scope&& scope )
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		test_scopes_.emplace_back( std::move( scope ) );
	}

	template< typename Promise >
	void _run( Promise&& promise )
	{
		promise
		.then( [ this ]( )
		{
			// Await all other promises which have been added for
			// awaiting.
			return await_all( );
		} )
		.fail( [ ]( std::exception_ptr e )
		{
			// If a test throws an asynchronous exception, the test
			// fails.
			ADD_FAILURE( )
				<< "Test threw unhandled asychronous exception:"
				<< std::endl
				<< q::stream_exception( e );
		} )
		.finally( [ this ]( )
		{
			bd->dispatcher( )->terminate( q::termination::linger );
		} );

		bd->dispatcher( )->start( );
	}

	// We only allow to run promises by r-value reference (i.e. we take
	// ownership).
	template< typename Promise >
	typename std::enable_if<
		q::is_promise< typename std::decay< Promise >::type >::value
		and
		!Promise::shared_type::value
		and
		!std::is_lvalue_reference< Promise >::value
	>::type
	run( Promise&& promise )
	{
		_run( std::move( promise ) );
	}

	// Shared promises, we can take as r-value references or copies.
	template< typename Promise >
	typename std::enable_if<
		q::is_promise< typename std::decay< Promise >::type >::value
		and
		Promise::shared_type::value
	>::type
	run( Promise&& promise )
	{
		_run( std::forward< Promise >( promise ) );
	}

	q::specific_execution_context_ptr< q::blocking_dispatcher > bd;
	q::specific_execution_context_ptr< q::threadpool > tp;

	q::queue_ptr queue;
	q::queue_ptr tp_queue;

	q::test::spy spy;

private:
	q::promise< std::tuple< > > await_all( )
	{
		std::vector< q::promise< std::tuple< > > > promises_;
		{
			Q_AUTO_UNIQUE_LOCK( mutex_ );
			std::swap( promises_, awaiting_promises_ );
		}

		if ( promises_.empty( ) )
			return q::with( queue );

		return q::all( promises_, queue )
		.then( [ this ]( )
		{
			// Recurse and try to await more promises which might
			// have been added since last time.
			return await_all( );
		} );
	}

	q::mutex mutex_;
	std::vector< q::promise< std::tuple< > > > awaiting_promises_;

	q::scope scope_;
	std::vector< q::scope > test_scopes_;
};

} } // namespace test, namespace q

#endif // LIBQ_TEST_FIXTURE_HPP
