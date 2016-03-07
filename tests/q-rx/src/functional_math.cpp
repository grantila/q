
#include <q-rx/functional.hpp>

#include "q-rx-test.hpp"

static double pi = 3.14159265358979323846264338327950288;

TEST( FunctionalMath, mul )
{
	auto mul_with_five = q::rx::f::mul( 5 );
	EXPECT_EQ( 0, mul_with_five( 0 ) );
	EXPECT_EQ( 5, mul_with_five( 1 ) );
	EXPECT_EQ( 10, mul_with_five( 2 ) );

	auto mul_with_pi = q::rx::f::mul( pi );
	double twenty_pi = double( 20 ) * pi;
	EXPECT_DOUBLE_EQ( twenty_pi, mul_with_pi( 20 ) );
}

TEST( FunctionalMath, div )
{
	auto div_by_five = q::rx::f::div( 5 );
	EXPECT_EQ( 3, div_by_five( 15 ) );
	EXPECT_EQ( 3, div_by_five( 16 ) );
	EXPECT_EQ( 3, div_by_five( 17 ) );
	EXPECT_EQ( 3, div_by_five( 18 ) );
	EXPECT_EQ( 3, div_by_five( 19 ) );
	EXPECT_EQ( 4, div_by_five( 20 ) );

	auto div_by_pi = q::rx::f::div( pi );
	double twenty_over_pi = double( 20 ) / pi;
	EXPECT_DOUBLE_EQ( twenty_over_pi, div_by_pi( 20 ) );
}

TEST( FunctionalMath, fast_mul_pi )
{
	auto thousand_pi = q::rx::f::fast_mul_pi< std::uint16_t >(
		1000 );
	EXPECT_EQ( 3141, thousand_pi );

	auto million_pi = q::rx::f::fast_mul_pi< std::uint32_t >(
		1000000 );
	EXPECT_EQ( 3141592, million_pi );

	auto billion_pi_32 = q::rx::f::fast_mul_pi< std::uint32_t >(
		1000000000 );
	EXPECT_EQ( 3141592920, billion_pi_32 );

	auto billion_pi_64 = q::rx::f::fast_mul_pi< std::uint64_t >(
		1000000000 );
	EXPECT_EQ( 3141592920, billion_pi_64 );
}

TEST( FunctionalMath, fast_div_pi )
{
	auto thousand_over_pi = q::rx::f::fast_div_pi< std::uint16_t >(
		1000 );
	EXPECT_EQ( 318, thousand_over_pi );

	auto million_over_pi = q::rx::f::fast_div_pi< std::uint32_t >(
		1000000 );
	EXPECT_EQ( 318309, million_over_pi );

	auto billion_over_pi_32 = q::rx::f::fast_div_pi< std::uint32_t >(
		1000000000 );
	EXPECT_EQ( 318309859, billion_over_pi_32 );

	auto billion_over_pi_64 = q::rx::f::fast_div_pi< std::uint64_t >(
		1000000000 );
	EXPECT_EQ( 318309859, billion_over_pi_64 );
}
