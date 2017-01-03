
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

	static std::uint16_t get_next_port( )
	{
		static struct qio_unit_tests_init
		{
			qio_unit_tests_init( )
			: ports( 1030 )
			{ }

			std::atomic< std::uint16_t > ports;
		} __qio_unit_tests_init;

		return __qio_unit_tests_init.ports++;
	}

private:
	void on_setup( ) override
	{
		std::tie( io_dispatcher, io_queue ) =
			q::make_event_dispatcher_and_queue< q::io::dispatcher >(
				queue );

		// When we start the io_dispatcher, we need to await its
		// completion before we actually run the tests.
		// We do this using the q-test fixture's blocking_dispatcher.
		io_dispatcher->start( )
		.then( [ this ]( )
		{
			bd->terminate( q::termination::linger );
		} );

		bd->start( );
		bd->await_termination( );
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
