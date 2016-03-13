
#include <q-test/q-test.hpp>

#include <q/lib.hpp>

int main( int argc, char** argv )
{
	q::settings settings;
	settings.set_long_stack_support( true );
	auto scope = std::move( q::scoped_initialize( settings ) );

	::testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS( );
}
