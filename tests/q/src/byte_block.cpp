
#include <q/type_traits.hpp>

#include <q-test/q-test.hpp>

#include <q/block.hpp>








/*


	byte_block( const std::string& s );
	byte_block( std::size_t size, std::uint8_t const* data );
	byte_block(
		std::size_t size, std::shared_ptr< const std::uint8_t > data );

	void advance( std::size_t amount );

	std::size_t size( ) const;

	std::uint8_t const* data( ) const;

	byte_block slice( std::size_t offset, std::size_t length ) const;
	byte_block slice( std::size_t offset ) const;

	byte_block slice_printable_ascii( ) const;
	byte_block slice_printable_ascii( std::size_t max_length ) const;

	std::string to_string( ) const;


*/





TEST( byte_block, empty )
{
	q::byte_block b;

	EXPECT_EQ( b.size( ), std::size_t( 0 ) );
}

TEST( byte_block, non_empty )
{
	q::byte_block b( 8, new std::uint8_t[ 8 ]{ 1, 2, 3, 4, 5, 6, 7, 8 } );

	EXPECT_EQ( b.size( ), std::size_t( 8 ) );
}

TEST( byte_block, slice_empty )
{
	q::byte_block b;
	auto b_slice = b.slice( 0 );

	EXPECT_EQ( b.size( ), std::size_t( 0 ) );
	EXPECT_EQ( b_slice.size( ), std::size_t( 0 ) );
}

TEST( byte_block, slice_non_empty )
{
	q::byte_block b( 8, new std::uint8_t[ 8 ]{ 1, 2, 3, 4, 5, 6, 7, 8 } );
	auto b_slice_1 = b.slice( 0 );
	auto b_slice_2 = b.slice( 0, 4 );
	auto b_slice_3 = b.slice( 4 );
	auto b_slice_4 = b.slice( 4, 4 );

	EXPECT_EQ( b.size( ), std::size_t( 8 ) );
	EXPECT_EQ( b_slice_1.size( ), std::size_t( 8 ) );
	EXPECT_EQ( b_slice_2.size( ), std::size_t( 4 ) );
	EXPECT_EQ( b_slice_3.size( ), std::size_t( 4 ) );
	EXPECT_EQ( b_slice_4.size( ), std::size_t( 4 ) );

	EXPECT_EQ( b_slice_1.data( )[ 0 ], std::size_t( 1 ) );
	EXPECT_EQ( b_slice_2.data( )[ 0 ], std::size_t( 1 ) );
	EXPECT_EQ( b_slice_3.data( )[ 0 ], std::size_t( 5 ) );
	EXPECT_EQ( b_slice_4.data( )[ 0 ], std::size_t( 5 ) );
}

TEST( byte_block, slice_ascii )
{
	q::byte_block b( 5, new std::uint8_t[ 5 ]{ 'f', 'o', 'o', 4, '!' } );
	auto b_slice = b.slice_printable_ascii( );

	EXPECT_EQ( b_slice.to_string( ), "foo" );

	q::byte_block b2( 8, new std::uint8_t[ 8 ]{
		'f', 'o', 'o', '\r', '\n', 'b', 'a', 'r'
	} );
	auto b2_slice = b2.slice_printable_ascii( );

	EXPECT_EQ( b2_slice.to_string( ), "foo\r\nbar" );
}
