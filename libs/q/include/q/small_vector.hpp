/*
 * Copyright 2017 Gustaf Räntilä
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

#ifndef LIBQ_SMALL_VECTOR_HPP
#define LIBQ_SMALL_VECTOR_HPP

#include <array>
#include <vector>

namespace q {

template< typename T, std::size_t N >
class small_vector
{
public:
	class iterator
	{
	public:
		iterator( small_vector& vec, std::size_t index )
		: vector_( vec )
		, index_( index )
		{ }

		iterator( ) = delete;
		iterator( const iterator& ) = default;
		iterator( iterator&& ) = default;

		iterator& operator=( const iterator& ) = default;
		iterator& operator=( iterator&& ) = default;

		bool operator!=( iterator other )
		{
			return index_ != other.index_;
		}

		T& operator*( )
		{
			return vector_[ index_ ];
		}

		T& operator->( )
		{
			return vector_[ index_ ];
		}

		iterator operator++( int )
		{
			iterator ret{ *this };
			++index_;
			return ret;
		}

		iterator& operator++( )
		{
			++index_;
			return *this;
		}

	private:
		small_vector& vector_;
		std::size_t index_;
	};

	small_vector( )
	: array_count_( 0 )
	{ }

	small_vector( const small_vector& ) = default;
	small_vector( small_vector&& ) = default;

	small_vector& operator=( const small_vector& ) = default;
	small_vector& operator=( small_vector&& ) = default;

	void push_back( T t )
	{
		if ( array_count_ < N )
			array_[ array_count_++ ] = std::move( t );
		else
			vector_.push_back( std::move( t ) );
	}

	void clear( )
	{
		std::uninitialized_fill(
			array_.begin( ),
			array_.begin( ) + array_count_,
			T( )
		);

		array_count_ = 0;
		vector_.clear( );
	}

	T& operator[ ]( std::size_t index )
	{
		if ( index < array_count_ )
			return array_[ index ];
		return vector_[ index - N ];
	}

	std::size_t size( ) const
	{
		if ( array_count_ < N )
			return array_count_;
		return N + vector_.size( );
	}

	iterator begin( )
	{
		return iterator{ *this, 0 };
	}

	iterator end( )
	{
		return iterator{ *this, size( ) };
	}

	bool empty( )
	{
		return array_count_ == 0;
	}

	template< typename Fn >
	void for_each( Fn&& fn )
	{
		for ( std::size_t i = 0; i < array_count_; ++i )
			fn( array_[ i ] );
		for ( auto& value : vector_ )
			fn( value );
	}

private:
	std::size_t array_count_;
	std::array< T, N > array_;
	std::vector< T > vector_;
};

} // namespace q

#endif // LIBQ_SMALL_VECTOR_HPP
