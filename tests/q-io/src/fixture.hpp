
#ifndef LIBQ_IO_FIXTURE_HPP
#define LIBQ_IO_FIXTURE_HPP

#include <q-io/dispatcher.hpp>

#include <q-test/q-test.hpp>
#include <q-test/expect.hpp>

#define QIO_TEST_MAKE_SCOPE( name ) \
	class name : public q::io::test::fixture { }

namespace q { namespace io { namespace test {

class fixture
: public ::q::test::fixture
{
public:
	fixture( )
	{ }

private:
	void on_setup( ) override
	{
		std::tie( io_dispatcher, io_queue ) =
			q::make_event_dispatcher_and_queue< q::io::dispatcher >(
				queue );

		io_dispatcher->start( );
	}

	void on_teardown( ) override
	{
		io_queue.reset( );
		io_dispatcher->terminate( dispatcher_termination::graceful );
		io_dispatcher->await_termination( );
		io_dispatcher.reset( );
	}

protected:
	q::io::dispatcher_ptr io_dispatcher;
	q::queue_ptr io_queue;
};

} } } // namespace test, namespace io, namespace q

#endif // LIBQ_IO_FIXTURE_HPP
