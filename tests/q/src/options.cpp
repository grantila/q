
#include <q/promise.hpp>

#include <gtest/gtest.h>

TEST( Options, foobar )
{
	q::options< q::concurrency, q::detail::set_default< q::queue_ptr >, q::queue_ptr > opts;
	//q::options< int, std::string > opts;

	auto opts1 = q::choose( );
	auto opts2 = opts1 | 4711;
	auto opts3 = opts2 | std::string( "asdf" );// | 17;

	//auto unknown = q::choose( ) | q::temporary( queue ) | queue;
	//opts = unknown;
}
