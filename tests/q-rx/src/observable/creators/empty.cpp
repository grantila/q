
#include "../../q-rx-test.hpp"

Q_TEST_MAKE_SCOPE( ob_create_empty );

TEST_F( ob_create_empty, def )
{
	auto o_empty = q::rx::observable< int >::empty( queue );

	run( o_empty.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ) );
}
