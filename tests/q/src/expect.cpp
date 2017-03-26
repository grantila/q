
#include <q/expect.hpp>

#include "core.hpp"

struct Copyable
{
	Copyable( ) = default;
	Copyable( const Copyable& ) = default;
	Copyable& operator=( const Copyable& ) = default;

	Copyable( Copyable&& ) = delete;
	Copyable& operator=( Copyable&& ) = delete;
};

struct Movable
{
	Movable( ) = default;
	Movable( Movable&& ) = default;
	Movable& operator=( Movable&& ) = default;

	Movable( const Movable& ) = delete;
	Movable& operator=( const Movable& ) = delete;
};

Q_MAKE_SIMPLE_EXCEPTION( test_exception );

std::exception_ptr make_exception( )
{
	return std::make_exception_ptr( test_exception( ) );
}
std::exception_ptr make_null_exception( )
{
	return std::exception_ptr( );
}

TEST( expect, void_expect_with_null_exception )
{
	EXPECT_THROW(
		q::refuse< void >( make_null_exception( ) ),
		q::invalid_exception_exception );
}

TEST( expect, void_expect_With_exception )
{
	auto expect = q::refuse< void >( make_exception( ) );
	EXPECT_TRUE( expect.has_exception( ) );
	EXPECT_NO_THROW( expect.exception( ) );
	EXPECT_THROW( expect.get( ), test_exception );
	EXPECT_THROW( expect.consume( ), test_exception );
}

TEST( expect, copyable_expect_with_null_exception )
{
	EXPECT_THROW(
		q::refuse< Copyable >( make_null_exception( ) ),
		q::invalid_exception_exception );
}

TEST( expect, copyable_expect_with_exception )
{
	auto expect = q::refuse< Copyable >( make_exception( ) );
	EXPECT_TRUE( expect.has_exception( ) );
	EXPECT_NO_THROW( expect.exception( ) );
	EXPECT_THROW( expect.get( ), test_exception );
	EXPECT_THROW( expect.consume( ), test_exception );
}

TEST( expect, movable_expect_with_null_exception )
{
	EXPECT_THROW(
		q::refuse< Movable >( make_null_exception( ) ),
		q::invalid_exception_exception );
}

TEST( expect, movable_expect_with_exception )
{
	auto expect = q::refuse< Movable >( make_exception( ) );
	EXPECT_TRUE( expect.has_exception( ) );
	EXPECT_NO_THROW( expect.exception( ) );
	EXPECT_THROW( expect.get( ), test_exception );
	EXPECT_THROW( expect.consume( ), test_exception );
}

TEST( expect, void_expect_with_value )
{
	auto expect = q::fulfill< void >( );
	EXPECT_FALSE( expect.has_exception( ) );
	EXPECT_EQ( expect.exception( ), std::exception_ptr( ) );
	EXPECT_NO_THROW( expect.get( ) );
	EXPECT_NO_THROW( expect.consume( ) );
}

TEST( expect, copyable_expect_with_value )
{
	auto expect = q::fulfill< Copyable >( Copyable( ) );
	EXPECT_FALSE( expect.has_exception( ) );
	EXPECT_EQ( expect.exception( ), std::exception_ptr( ) );
	EXPECT_NO_THROW( expect.get( ) );
	EXPECT_NO_THROW( expect.consume( ) );
}

TEST( expect, movable_expect_with_value )
{
	auto expect = q::fulfill< Movable >( Movable( ) );
	EXPECT_FALSE( expect.has_exception( ) );
	EXPECT_EQ( expect.exception( ), std::exception_ptr( ) );
	EXPECT_NO_THROW( expect.get( ) );
	EXPECT_NO_THROW( expect.consume( ) );
}

TEST( expect, any_expect_with_value )
{
	auto expect = q::fulfill< std::string >( std::string( ) );
	EXPECT_FALSE( expect.has_exception( ) );
	EXPECT_EQ( expect.exception( ), std::exception_ptr( ) );
	EXPECT_NO_THROW( expect.get( ) );
	EXPECT_NO_THROW( expect.consume( ) );
}

TEST( expect, exception_expect_with_value )
{
	auto expect = q::fulfill< std::exception_ptr >( make_exception( ) );
	EXPECT_FALSE( expect.has_exception( ) );
	EXPECT_EQ( expect.exception( ), std::exception_ptr( ) );
	EXPECT_NO_THROW( expect.get( ) );
	EXPECT_NO_THROW( expect.consume( ) );
}

TEST( expect, exception_expect_with_null_value )
{
	auto expect = q::fulfill< std::exception_ptr >( make_null_exception( ) );
	EXPECT_FALSE( expect.has_exception( ) );
	EXPECT_EQ( expect.exception( ), std::exception_ptr( ) );
	EXPECT_NO_THROW( expect.get( ) );
	EXPECT_NO_THROW( expect.consume( ) );
}

TEST( expect, exception_expect_with_exception )
{
	auto expect = q::refuse< std::exception_ptr >( make_exception( ) );
	EXPECT_TRUE( expect.has_exception( ) );
	EXPECT_NE( expect.exception( ), std::exception_ptr( ) );
	EXPECT_THROW( expect.get( ), test_exception );
	EXPECT_THROW( expect.consume( ), test_exception );
}

TEST( expect, exception_expect_with_null_exception )
{
	EXPECT_THROW(
		q::refuse< std::exception_ptr >( make_null_exception( ) ),
		q::invalid_exception_exception );
}
