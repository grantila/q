
#include <q-test/q-test.hpp>

#include <q/lib.hpp>
#include <q/static_atomic.hpp>

namespace {

static struct q_initialization
{
	q_initialization( )
	: scope_( q::make_scope( 0 ) )
	{
		q::settings settings;
		settings.set_long_stack_support( true );

		scope_ = std::move( q::scoped_initialize( settings ) );
	}

	~q_initialization( )
	{
	}

private:
	q::scope scope_;
} q_initialization_dummy_;

} // anonymous namespace
