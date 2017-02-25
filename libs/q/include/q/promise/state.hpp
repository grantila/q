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

template< typename T >
struct promise_state_base
{
	typedef expect< T > value_type;

	promise_signal& signal( )
	{
		return signal_;
	}

	value_type& get( )
	{
		return *reinterpret_cast< value_type* >( &value_ );
	}

	void set( value_type&& value )
	{
		{
			Q_AUTO_UNIQUE_LOCK( signal_.mutex_ );

			if ( signal_.done_ )
				// Don't set the value twice.
				// TODO: Consider throwing
				return;

			::new ( &value_ ) value_type( std::move( value ) );
		}

		signal_.done( );
	}

private:
	typename std::aligned_storage< sizeof( value_type ) >::type value_;
	promise_signal signal_;
};

template< typename T, bool Shared > // Shared == false
struct promise_state
{
	promise_state( std::shared_ptr< promise_state_base< T > >&& state )
	: state_( std::move( state ) )
	{ }

	expect< T > consume( )
	{
		return std::move( state_->get( ) );
	}

	const expect< T >& get( )
	{
		return state_->get( );
	}

	promise_signal& signal( )
	{
		return state_->signal( );
	}

	std::shared_ptr< promise_state_base< T > > state( )
	{
		return state_;
	}

private:
	std::shared_ptr< promise_state_base< T > > state_;
};

template< typename T >
struct promise_state< T, true >
{
	promise_state( std::shared_ptr< promise_state_base< T > >&& state )
	: state_( std::move( state ) )
	{ }

	expect< T > consume( )
	{
		return state_->get( );
	}

	expect< T > get( )
	{
		return state_->get( );
	}

	promise_signal& signal( )
	{
		return state_->signal( );
	}

	std::shared_ptr< promise_state_base< T > > state( )
	{
		return state_;
	}

private:
	std::shared_ptr< promise_state_base< T > > state_;
};

} } // namespace detail, namespace queue

#endif // LIBQ_PROMISE_STATE_HPP
