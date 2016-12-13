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

#include <cstring>

namespace q {

namespace {

template< typename Deleter >
std::shared_ptr< const std::uint8_t >
wrap_shared( std::uint8_t const* ptr, Deleter&& deleter )
{
	return std::shared_ptr< const std::uint8_t >(
		ptr, std::forward< Deleter >( deleter ) );
}

std::shared_ptr< const std::uint8_t > wrap_shared( std::uint8_t const* ptr )
{
	return wrap_shared( ptr, [ ]( std::uint8_t const* ptr )
	{
		delete[ ] ptr;
	} );
}

std::shared_ptr< const std::uint8_t > alloc_shared( std::size_t size )
{
	auto xdata = q::make_unique< std::uint8_t[ ] >( size );
	auto data = std::unique_ptr< std::uint8_t const[ ] >( std::move( xdata ) );
	auto shared_data = wrap_shared( data.get( ), data.get_deleter( ) );
	data.release( );
	return shared_data;
}

} // anonymous namespace

byte_block::byte_block( )
: size_( 0 )
, ptr_( nullptr )
{ }

byte_block::byte_block( const std::string& s )
: size_( s.size( ) )
, data_( alloc_shared( size_ ) )
, ptr_( data_.get( ) )
{
	std::memcpy( const_cast< std::uint8_t* >( ptr_ ), s.data( ), size_ );
}

byte_block::byte_block( std::size_t size, std::uint8_t const* data )
: size_( size )
, data_( wrap_shared( data ) )
, ptr_( data_.get( ) )
{ }

byte_block::byte_block(
	std::size_t size, std::shared_ptr< const std::uint8_t > data
)
: size_( size )
, data_( data )
, ptr_( data_.get( ) )
{ }

void byte_block::advance( std::size_t amount )
{
	if ( amount > size_ )
		Q_THROW( std::out_of_range(
			"byte_block::advance cannot advance out of buffer" ) );

	ptr_ += amount;
	size_ -= amount;
}

std::size_t byte_block::size( ) const
{
	return size_;
}

std::uint8_t const* byte_block::data( ) const
{
	return ptr_;
}

std::string byte_block::to_string( ) const
{
	return std::string( reinterpret_cast< const char* >( ptr_ ), size( ) );
}

} // namespace q
