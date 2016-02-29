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
	byte_block( std::size_t size );

	void resize( std::size_t new_size );
	void advance( std::size_t amount );

	std::size_t size( ) const;

	std::uint8_t* data( );
	std::uint8_t const* data( ) const;

	/**
	 * Converts this byte block to a string. NOTE; This is not safe for
	 * strings with ascii zero, and is not safe for non-8-bit strings (such
	 * as UTF-8).
	 */
	std::string to_string( ) const;

private:
	std::vector< std::uint8_t > data_;
	std::size_t offset_;
	std::size_t size_;
};

} // namespace q

#endif // LIBQ_BLOCK_HPP
