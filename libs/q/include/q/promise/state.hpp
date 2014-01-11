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

#ifndef LIBQ_PROMISE_STATE_HPP
#define LIBQ_PROMISE_STATE_HPP

#include <future>

// TODO: Consider moving to is_nothrow_* alternatives since we won't allow
// exceptions to be thrown when copying or moving data between asynchronous
// tasks.

namespace q { namespace detail {

template< typename T, bool Shared >
struct promise_state_data
{
	typedef expect< T > value_type;
	typedef std::future< value_type > future_type;

	promise_state_data( ) = delete;
	promise_state_data( promise_state_data< T, Shared >&& ) = default;
	promise_state_data( const promise_state_data< T, Shared >& ) = delete;

	promise_state_data( future_type&& future )
	: future( std::move( future ) )
	, signal( make_shared< promise_signal >( ) )
	{ }

	future_type future;
	promise_signal_ptr signal;
};

template< typename T >
struct promise_state_data< T, true >
{
	typedef expect< T > value_type;
	typedef std::shared_future< value_type > future_type;

	promise_state_data( ) = delete;
	promise_state_data( promise_state_data< T, true >&& unique ) = default;
	promise_state_data( const promise_state_data< T, true >& unique ) = default;

	promise_state_data( promise_state_data< T, false >&& unique )
	: future( unique.future.share( ) )
	, signal( unique.signal )
	{ }

	future_type future;
	promise_signal_ptr signal;
};

template< typename T >
struct shared_state
{
	typedef promise_state_data< T, true > state_type;

public:
	shared_state( ) = delete;
	shared_state( const shared_state< T >& ) = default;
	shared_state( shared_state< T >&& ) = default;

	typename state_type::value_type consume( )
	{
		return data_->future.get( );
	}

	promise_signal_ptr signal( )
	{
		return data_->signal;
	}

	state_type acquire( )
	{
		return *data_;
	}

protected:
	shared_state( promise_state_data< T, false >&& data )
	: data_( std::make_shared< state_type >( std::move( data ) ) )
	{ }

private:
	std::shared_ptr< state_type > data_;
};

template< typename T >
class unique_state
{
	typedef promise_state_data< T, false > state_type;

public:
	unique_state( ) = delete;
	unique_state( const unique_state< T >& ) = delete;
	unique_state( unique_state< T >&& s )// = default;
	: data_( std::move( s.data_ ) )
	{ }

	typename state_type::value_type& ref( )
	{
		return data_.future.get( );
	}

	typename state_type::value_type consume( )
	{
		return std::move( data_.future.get( ) );
	}

	promise_signal_ptr signal( )
	{
		return data_.signal;
	}

	state_type acquire( )
	{
		return std::move( data_ );
	}

protected:
	unique_state( state_type&& data )
	: data_( std::move( data ) )
	{ }

private:
	state_type data_;
};

template< typename T, bool Shared >
struct promise_state;

template< typename T >
struct promise_state< T, true >
: shared_state< T >
{
	typedef shared_state< T > base_type;

	promise_state( promise_state_data< T, false >&& data )
	: base_type( std::move( data ) )
	{ }

	promise_state( promise_state< T, true >&& data ) = default;
	promise_state( const promise_state< T, true >& data ) = default;

	static_assert(
		is_copyable_or_movable< T >::value,
		"T must be copyable or movable" );
};

template< typename T >
struct promise_state< T, false >
: unique_state< T >
{
	typedef unique_state< T > base_type;

	promise_state( promise_state_data< T, false >&& data )
	: base_type( std::move( data ) )
	{ }

	promise_state( promise_state< T, false >&& data ) = default;
	promise_state( const promise_state< T, false >& data ) = delete;

	static_assert( is_movable< T >::value, "T must be movable" );
};

} } // namespace detail, namespace queue

#endif // LIBQ_PROMISE_STATE_HPP
