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

#include <q/block.hpp>
#include <q/exception.hpp>

namespace q {

byte_block::byte_block( )
: offset_( 0 )
, size_( 0 )
{ }

byte_block::byte_block( const std::string& s )
: offset_( 0 )
, size_( s.size( ) )
{
	data_.resize( size_ );
	memcpy( &data_[ 0 ], s.data( ), size_ );
}

byte_block::byte_block( std::size_t size )
: offset_( 0 )
, size_( size )
{
	data_.resize( size, 0 );
}

void byte_block::resize( std::size_t new_size )
{
	if ( new_size > size_ )
		Q_THROW( std::out_of_range(
			"byte_block::resize can only shrink, not grow" ) );

	size_ = new_size;
}

void byte_block::advance( std::size_t amount )
{
	if ( amount > size_ )
		Q_THROW( std::out_of_range(
			"byte_block::advance cannot advance out of buffer" ) );

	offset_ += amount;
	size_ -= amount;
}

std::size_t byte_block::size( ) const
{
	return size_;
}

std::uint8_t* byte_block::data( )
{
	return &data_[ 0 ];
}

std::uint8_t const* byte_block::data( ) const
{
	return &data_[ 0 ];
}

std::string byte_block::to_string( ) const
{
	return std::string(
		reinterpret_cast< const char* >( &data_[ 0 ] + offset_ ),
		size( ) );
}

} // namespace q
