/*
 * Copyright 2016 Gustaf Räntilä
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBQ_BLOCK_HPP
#define LIBQ_BLOCK_HPP

#include <vector>
#include <string>

namespace q {

class byte_block
{
public:
	byte_block( );
	byte_block( const std::string& s );
	byte_block( std::size_t size, std::uint8_t const* data );
	byte_block(
		std::size_t size, std::shared_ptr< const std::uint8_t > data );

	void advance( std::size_t amount );

	std::size_t size( ) const;

	std::uint8_t const* data( ) const;

	/**
	 * Returns a new byte_block which is a slice of this byte_block.
	 * Will throw `std::out_of_range` if the offset and length aren't
	 * within the boundaries of this byte_block.
	 *
	 * If `length` is not provided, it will use the entire length of the
	 * byte_block.
	 */
	byte_block slice( std::size_t offset, std::size_t length ) const;
	byte_block slice( std::size_t offset ) const;

	/**
	 * Slices this byte_block to a new byte_block from first byte up to but
	 * not including the first byte which is not printable ASCII.
	 * This may be a zero sized byte_block.
	 */
	byte_block slice_printable_ascii( ) const;
	byte_block slice_printable_ascii( std::size_t max_length ) const;

	/**
	 * Converts this byte block to a string. NOTE; This is not safe for
	 * strings with ascii zero, and is not safe for non-8-bit strings (such
	 * as UTF-8).
	 */
	std::string to_string( ) const;

private:
	byte_block(
		std::size_t offset,
		std::size_t size,
		std::shared_ptr< const std::uint8_t > data
	);

	std::size_t size_;
	std::shared_ptr< const std::uint8_t > data_;
	std::uint8_t const* ptr_;
};

} // namespace q

#endif // LIBQ_BLOCK_HPP
