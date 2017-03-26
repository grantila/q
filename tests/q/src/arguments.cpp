
#include <q/type_traits.hpp>

#include "core.hpp"

TEST( arguments, empty_is_empty )
{
	EXPECT_TRUE(
		q::arguments< >::equals< q::arguments< > >::value
	);
}

TEST( arguments, empty_is_convertible_to_empty )
{
	EXPECT_TRUE(
		q::arguments< >::is_convertible_to< q::arguments< > >::value
	);
}
