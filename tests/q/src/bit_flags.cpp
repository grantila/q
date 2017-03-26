
#include <q/type_traits.hpp>

#include "core.hpp"

#include <q/bit_flags.hpp>

TEST( bit_flags, empty )
{
	typedef q::bit_flags< 0 > flags_type;

	flags_type flags;

	EXPECT_THROW( flags.set( 0 ), std::out_of_range );
	EXPECT_THROW( flags.set( 0, false ), std::out_of_range );
	EXPECT_THROW( flags.unset( 0 ), std::out_of_range );
	EXPECT_THROW( flags.is_set( 0 ), std::out_of_range );
}

TEST( bit_flags, one_bit )
{
	typedef q::bit_flags< 1 > flags_type;

	flags_type flags;

	EXPECT_FALSE( flags.is_set( 0 ) );
	flags.set( 0 );
	EXPECT_TRUE( flags.is_set( 0 ) );
	flags.unset( 0 );
	EXPECT_FALSE( flags.is_set( 0 ) );

	EXPECT_THROW( flags.set( 1 ), std::out_of_range );
	EXPECT_THROW( flags.set( 1, false ), std::out_of_range );
	EXPECT_THROW( flags.unset( 1 ), std::out_of_range );
	EXPECT_THROW( flags.is_set( 1 ), std::out_of_range );
}

TEST( bit_flags, eight_bits )
{
	typedef q::bit_flags< 8 > flags_type;

	flags_type flags;

	EXPECT_FALSE( flags.is_set( 0 ) );
	EXPECT_FALSE( flags.is_set( 1 ) );
	EXPECT_FALSE( flags.is_set( 2 ) );
	EXPECT_FALSE( flags.is_set( 3 ) );
	EXPECT_FALSE( flags.is_set( 4 ) );
	EXPECT_FALSE( flags.is_set( 5 ) );
	EXPECT_FALSE( flags.is_set( 6 ) );
	EXPECT_FALSE( flags.is_set( 7 ) );
	flags.set( 0 );
	EXPECT_TRUE( flags.is_set( 0 ) );
	flags.unset( 0 );
	EXPECT_FALSE( flags.is_set( 0 ) );
	flags.set( 7 );
	EXPECT_TRUE( flags.is_set( 7 ) );
	flags.unset( 7 );
	EXPECT_FALSE( flags.is_set( 7 ) );

	EXPECT_THROW( flags.set( 8 ), std::out_of_range );
	EXPECT_THROW( flags.set( 8, false ), std::out_of_range );
	EXPECT_THROW( flags.unset( 8 ), std::out_of_range );
	EXPECT_THROW( flags.is_set( 8 ), std::out_of_range );
}

TEST( bit_flags, nine_bits )
{
	typedef q::bit_flags< 9 > flags_type;

	flags_type flags;

	EXPECT_FALSE( flags.is_set( 0 ) );
	EXPECT_FALSE( flags.is_set( 1 ) );
	EXPECT_FALSE( flags.is_set( 2 ) );
	EXPECT_FALSE( flags.is_set( 3 ) );
	EXPECT_FALSE( flags.is_set( 4 ) );
	EXPECT_FALSE( flags.is_set( 5 ) );
	EXPECT_FALSE( flags.is_set( 6 ) );
	EXPECT_FALSE( flags.is_set( 7 ) );
	EXPECT_FALSE( flags.is_set( 8 ) );
	flags.set( 0 );
	EXPECT_TRUE( flags.is_set( 0 ) );
	flags.unset( 0 );
	EXPECT_FALSE( flags.is_set( 0 ) );
	flags.set( 7 );
	EXPECT_TRUE( flags.is_set( 7 ) );
	flags.unset( 7 );
	EXPECT_FALSE( flags.is_set( 7 ) );
	flags.set( 8 );
	EXPECT_TRUE( flags.is_set( 8 ) );
	flags.unset( 8 );
	EXPECT_FALSE( flags.is_set( 8 ) );

	EXPECT_THROW( flags.set( 9 ), std::out_of_range );
	EXPECT_THROW( flags.set( 9, false ), std::out_of_range );
	EXPECT_THROW( flags.unset( 9 ), std::out_of_range );
	EXPECT_THROW( flags.is_set( 9 ), std::out_of_range );
}


TEST( bit_flags, compile_time_nine_bits )
{
	typedef q::bit_flags< 9 > flags_type;

	flags_type flags;

	EXPECT_FALSE( flags.is_set< 0 >( ) );
	EXPECT_FALSE( flags.is_set< 1 >( ) );
	EXPECT_FALSE( flags.is_set< 2 >( ) );
	EXPECT_FALSE( flags.is_set< 3 >( ) );
	EXPECT_FALSE( flags.is_set< 4 >( ) );
	EXPECT_FALSE( flags.is_set< 5 >( ) );
	EXPECT_FALSE( flags.is_set< 6 >( ) );
	EXPECT_FALSE( flags.is_set< 7 >( ) );
	EXPECT_FALSE( flags.is_set< 8 >( ) );
	flags.set< 0 >( );
	EXPECT_TRUE( flags.is_set< 0 >( ) );
	flags.unset< 0 >( );
	EXPECT_FALSE( flags.is_set< 0 >( ) );
	flags.set< 7 >( );
	EXPECT_TRUE( flags.is_set< 7 >( ) );
	flags.unset< 7 >( );
	EXPECT_FALSE( flags.is_set< 7 >( ) );
	flags.set< 8 >( );
	EXPECT_TRUE( flags.is_set< 8 >( ) );
	flags.unset< 8 >( );
	EXPECT_FALSE( flags.is_set< 8 >( ) );

	EXPECT_THROW( flags.set( 9 ), std::out_of_range );
	EXPECT_THROW( flags.set( 9, false ), std::out_of_range );
	EXPECT_THROW( flags.unset( 9 ), std::out_of_range );
	EXPECT_THROW( flags.is_set( 9 ), std::out_of_range );
}

TEST( bit_flags, empty_types )
{
	typedef q::bit_flags_of_types< > flags_type;

	flags_type flags;

	flags.set_by_type< >( );
	flags.unset_by_type< >( );

	/*
	// None if these should compile:
	flags.set_by_type< int >( );
	flags.set_by_type< int >( false );
	flags.unset_by_type< int >( );
	flags.is_set_by_type< int >( );
	/* */
}

TEST( bit_flags, one_type )
{
	typedef q::bit_flags_of_types< int > flags_type;

	flags_type flags;

	EXPECT_FALSE( flags.is_set_by_type< int >( ) );
	flags.set_by_type< int >( );
	EXPECT_TRUE( flags.is_set_by_type< int >( ) );
	flags.unset_by_type< int >( );
	EXPECT_FALSE( flags.is_set_by_type< int >( ) );
}

TEST( bit_flags, two_types )
{
	typedef q::bit_flags_of_types< int, bool > flags_type;

	flags_type flags;

	EXPECT_FALSE( flags.is_set_by_type< int >( ) );
	flags.set_by_type< int >( );
	EXPECT_TRUE( flags.is_set_by_type< int >( ) );
	flags.unset_by_type< int >( );
	EXPECT_FALSE( flags.is_set_by_type< int >( ) );

	EXPECT_FALSE( flags.is_set_by_type< bool >( ) );
	flags.set_by_type< bool >( );
	EXPECT_TRUE( flags.is_set_by_type< bool >( ) );
	flags.unset_by_type< bool >( );
	EXPECT_FALSE( flags.is_set_by_type< bool >( ) );
}

TEST( bit_flags, nine_types )
{
	typedef q::bit_flags_of_types<
		std::uint8_t,
		std::uint16_t,
		std::uint32_t,
		std::uint64_t,
		std::int8_t,
		std::int16_t,
		std::int32_t,
		std::int64_t,
		bool
	> flags_type;

	flags_type flags;

	EXPECT_FALSE( flags.is_set_by_type< std::uint8_t >( ) );
	flags.set_by_type< std::uint8_t >( );
	EXPECT_TRUE( flags.is_set_by_type< std::uint8_t >( ) );
	flags.unset_by_type< std::uint8_t >( );
	EXPECT_FALSE( flags.is_set_by_type< std::uint8_t >( ) );

	EXPECT_FALSE( flags.is_set_by_type< std::int64_t >( ) );
	flags.set_by_type< std::int64_t >( );
	EXPECT_TRUE( flags.is_set_by_type< std::int64_t >( ) );
	flags.unset_by_type< std::int64_t >( );
	EXPECT_FALSE( flags.is_set_by_type< std::int64_t >( ) );

	EXPECT_FALSE( flags.is_set_by_type< bool >( ) );
	flags.set_by_type< bool >( );
	EXPECT_TRUE( flags.is_set_by_type< bool >( ) );
	flags.unset_by_type< bool >( );
	EXPECT_FALSE( flags.is_set_by_type< bool >( ) );
}
