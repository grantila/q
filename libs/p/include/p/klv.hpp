/*
 * Copyright 2014 Gustaf Räntilä
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

#ifndef LIBP_KLV_HPP
#define LIBP_KLV_HPP

#include <p/endian.hpp>

#define LIBP_PACKED __attribute__(( __packed__ ))

namespace p {

namespace detail {

class klv_io
{
protected:
	virtual std::uint64_t key( ) const = 0;
	virtual std::uint64_t length( ) const = 0;
	inline std::uint8_t block_size( ) const
	{
		return length( ) + 16;
	}
	virtual void store( );
};

class LIBP_PACKED klv
{
public:
	klv( ) = delete;

	std::uint64_t key( ) const
	{
		return key_;
	}

	std::uint64_t length( ) const
	{
		return len_;
	}

	std::uint8_t* value( )
	{
		return static_cast< std::uint8_t* >( this ) + 16;
	}

protected:
	klv( std::uint64_t key, std::uint64_t len )
	: key_( key )
	, len_( len )
	{ }

	~klv( ) { }

private:
	le< std::uint64_t > key_;
	le< std::uint64_t > len_;
};

} // namespace detail

template< std::uint64_t Key, std::uint64_t Len >
class static_klv
: public detail::klv
{
public:
	static_klv( )
	: detail::klv( Key, Len )
	{ }
};

template< std::uint64_t Key >
struct dynamic_klv
{
public:
	dynamic_klv( std::uint64_t len )
	: detail::klv( Key, len )
	{ }
};

} // namespace p

#endif // LIBP_KLV_HPP
