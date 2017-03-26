
#include "../core.hpp"

Q_TEST_MAKE_SCOPE( expect_gt );

TEST_F( expect_gt, int_gt_int )
{
	EVENTUALLY_EXPECT_GT( 6, 5 );
}

TEST_F( expect_gt, async_int_gt_int )
{
	EVENTUALLY_EXPECT_GT( q::with( queue, 6 ), 5 );
}

TEST_F( expect_gt, int_gt_async_int )
{
	EVENTUALLY_EXPECT_GT( 6, q::with( queue, 5 ) );
}

TEST_F( expect_gt, async_int_gt_async_int )
{
	EVENTUALLY_EXPECT_GT( q::with( queue, 6 ), q::with( queue, 5 ) );
}
