
#include "../core.hpp"

Q_TEST_MAKE_SCOPE( expect_lt );

TEST_F( expect_lt, int_lt_int )
{
	EVENTUALLY_EXPECT_LT( 5, 6 );
}

TEST_F( expect_lt, async_int_lt_int )
{
	EVENTUALLY_EXPECT_LT( q::with( queue, 5 ), 6 );
}

TEST_F( expect_lt, int_lt_async_int )
{
	EVENTUALLY_EXPECT_LT( 5, q::with( queue, 6 ) );
}

TEST_F( expect_lt, async_int_lt_async_int )
{
	EVENTUALLY_EXPECT_LT( q::with( queue, 5 ), q::with( queue, 6 ) );
}
