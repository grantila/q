
#include <q-test/expect.hpp>

Q_TEST_MAKE_SCOPE( expect_promises );

TEST_F( expect_promises, eventual_resolution )
{
	auto p = q::with( queue, 6 ).share( );

	EVENTUALLY_EXPECT_RESOLUTION( p );
}

TEST_F( expect_promises, eventual_rejection )
{
	auto err = std::logic_error( "err" );
	auto p = q::reject< q::arguments< int > >( queue, err ).share( );

	EVENTUALLY_EXPECT_REJECTION( p );
}

TEST_F( expect_promises, eventual_rejection_with_exception )
{
	auto err = std::logic_error( "err" );
	auto p = q::reject< q::arguments< int > >( queue, err ).share( );

	EVENTUALLY_EXPECT_REJECTION( p );
	EVENTUALLY_EXPECT_REJECTION_WITH( p, std::logic_error );
}

TEST_F( expect_promises, shared_promise )
{
	EVENTUALLY_EXPECT_SHARED( q::with( queue, 6 ).share( ) );
}

TEST_F( expect_promises, unique_promise )
{
	EVENTUALLY_EXPECT_UNIQUE( q::with( queue, 6 ) );
}
