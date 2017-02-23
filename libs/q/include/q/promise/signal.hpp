/*
 * Copyright 2013 Gustaf Räntilä
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

#ifndef LIBQ_PROMISE_SIGNAL_HPP
#define LIBQ_PROMISE_SIGNAL_HPP

#include <q/types.hpp>
#include <q/mutex.hpp>

#include <memory>
#include <array>

namespace q { namespace detail {

namespace {

/**
 * items are tasks bound to a certain queue.
 *
 * They can also be synchronous tasks which run directly when the promise is
 * resolved. These must be tiny and fast and they must be 'noexcept'. This is
 * used for scheduling custom async tasks for event loops, e.g. timers.
 */
struct item
{
	task task_;
	queue_ptr queue_;
	bool synchronous_;
};

} // anonymous namespace

template< typename T, std::size_t N >
class small_vector
{
public:
	class iterator
	{
	public:
		iterator( small_vector& vec )
		: vector_( vec )
		, index_( vector_.size( ) )
		{ }

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
	: array_elements_( 0 )
	{ }

	small_vector( const small_vector& ) = default;
	small_vector( small_vector&& ) = default;

	small_vector& operator=( const small_vector& ) = default;
	small_vector& operator=( small_vector&& ) = default;

	void push_back( T t )
	{
		if ( array_elements_ < N )
			array_[ array_elements_++ ] = std::move( t );
		else
			vector_.push_back( std::move( t ) );
	}

	void clear( )
	{
		for ( std::size_t i = 0; i < array_elements_; ++i )
			array_[ i ].~T( );

		array_elements_ = 0;
		vector_.clear( );
	}

	T& operator[ ]( std::size_t index )
	{
		if ( array_elements_ < N )
			return array_[ index ];
		return vector_[ index - N ];
	}

	std::size_t size( ) const
	{
		if ( array_elements_ < N )
			return array_elements_;
		return N + vector_.size( );
	}

	iterator begin( )
	{
		return iterator{ *this, 0 };
	}

	iterator end( )
	{
		return iterator{ *this };
	}

private:
	std::size_t array_elements_;
	std::array< T, N > array_;
	std::vector< T > vector_;
};

// TODO: Make lock-free with a lock-free queue and atomic bool.
class promise_signal
: public std::enable_shared_from_this< promise_signal >
{
public:
	~promise_signal( );

	void done( ) noexcept;

	void push( task&& task, const queue_ptr& queue ) noexcept;
	void push_synchronous( task&& task ) noexcept;

protected:
	promise_signal( );

private:
	mutex mutex_;
	bool done_;
	small_vector< item, 1 > items_;
	//std::vector< item > items_;
};

typedef std::shared_ptr< promise_signal > promise_signal_ptr;

} } // namespace detail, namespace queue

#endif // LIBQ_PROMISE_SIGNAL_HPP
