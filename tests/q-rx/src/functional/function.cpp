
#include "../q-rx-test.hpp"

TEST( functional_function, append_result )
{
	auto fn = [ ]( int i )
	{
		return std::to_string( i );
	};

	auto wrapped = q::rx::f::append_result( fn );

	EXPECT_EQ( wrapped( 5 ), std::make_tuple( 5, std::string( "5" ) ) );
}
