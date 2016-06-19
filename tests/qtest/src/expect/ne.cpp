
#include <q-test/expect.hpp>

Q_TEST_MAKE_SCOPE( expect_ne );

TEST_F( expect_ne, int_ne_int )
{
	EVENTUALLY_EXPECT_NE( 5, 6 );
}

TEST_F( expect_ne, async_int_ne_int )
{
	EVENTUALLY_EXPECT_NE( q::with( queue, 5 ), 6 );
}

TEST_F( expect_ne, int_ne_async_int )
{
	EVENTUALLY_EXPECT_NE( 5, q::with( queue, 6 ) );
}

TEST_F( expect_ne, async_int_ne_async_int )
{
	EVENTUALLY_EXPECT_NE( q::with( queue, 5 ), q::with( queue, 6 ) );
}
