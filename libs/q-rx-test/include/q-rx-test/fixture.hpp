/*
 * Copyright 2016 Gustaf Räntilä
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

#ifndef LIBQ_RX_TEST_FIXTURE_HPP
#define LIBQ_RX_TEST_FIXTURE_HPP

#include <q-test/q-test.hpp>
#include <q-test/expect.hpp>

#define QIO_TEST_MAKE_SCOPE( name ) \
	class name : public q::rx::test::fixture { }

namespace q { namespace rx { namespace test {

class fixture
: public ::q::test::fixture
{
public:
	fixture( )
	{ }

private:
	void on_setup( ) override
	{
		ioc = q::make_execution_context< q::io::dispatcher >( queue );
		io_dispatcher = ioc->dispatcher( );
		io_queue = tp->queue( );

		io_dispatcher->start( );
	}

	void on_teardown( ) override
	{
		io_queue.reset( );
		io_dispatcher->terminate( dispatcher_termination::graceful );
		io_dispatcher->await_termination( );
		io_dispatcher.reset( );
		ioc.reset( );
	}

protected:
	q::specific_execution_context_ptr< q::io::dispatcher > ioc;
	q::io::dispatcher_ptr io_dispatcher;
	q::queue_ptr io_queue;
};

} } } // namespace test, namespace rx, namespace q

#endif // LIBQ_RX_TEST_FIXTURE_HPP
