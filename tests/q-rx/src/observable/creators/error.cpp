
#include "../../q-rx-test.hpp"

Q_TEST_MAKE_SCOPE( ob_create_error );

TEST_F( ob_create_error, def )
{
	auto o_error = q::rx::observable< int >::error( queue, Error( ) );

	EVENTUALLY_EXPECT_REJECTION_WITH(
		o_error.consume( EXPECT_NO_CALL_WRAPPER( [ ]( int ) { } ) ),
		Error
	);
}
