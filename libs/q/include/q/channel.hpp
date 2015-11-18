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

#ifndef LIBQ_CHANNEL_HPP
#define LIBQ_CHANNEL_HPP

#include <q/exception.hpp>
#include <q/mutex.hpp>
#include <q/promise.hpp>

#include <queue>
#include <atomic>

namespace q {

Q_MAKE_SIMPLE_EXCEPTION( channel_closed_exception );

template< typename... T >
class readable
{
public:
	typedef std::tuple< T... > tuple_type;

	virtual promise< tuple_type > receive( ) = 0;
};

template< typename... T >
class writable
{
public:
	typedef std::tuple< T... > tuple_type;

	void send( T&&... t )
	{
		return send( std::make_tuple( std::move( t )... ) );
	}

	void send( const T&... t )
	{
		return send( std::make_tuple( t... ) );
	}

	virtual void send( tuple_type&& t ) = 0;
};

template< typename... T >
class channel
: public std::enable_shared_from_this< channel< T... > >
, public readable< T... >
, public writable< T... >
{
public:
	typedef std::tuple< T... >    tuple_type;
	typedef detail::defer< T... > defer_type;
	typedef arguments< T... >     arguments_type;

	channel( /* q::location, name */ )
	: mutex_( Q_HERE, "channel" )
	, closed_( false )
	{ }

	void close( )
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		closed_.store( true, std::memory_order_seq_cst );
	}

	void send( tuple_type&& t ) override
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		if ( closed_.load( std::memory_order_seq_cst ) )
			Q_THROW( channel_closed_exception( ) );

		if ( waiters_.empty( ) )
		{
			queue_.push( std::move( t ) );
		}
		else
		{
			auto waiter = std::move( waiters_.front( ) );
			waiters_.pop( );

			waiter->set_value( std::move( t ) );
		}
	}

	promise< tuple_type > receive( ) override
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		if ( queue_.empty( ) )
		{
			if ( closed_.load( std::memory_order_seq_cst ) )
				return reject< arguments_type >(
					channel_closed_exception( ) );

			auto defer = ::q::make_shared< defer_type >( );

			waiters_.push( defer );

			return defer->get_promise( );
		}
		else
		{
			tuple_type t = std::move( queue_.front( ) );
			queue_.pop( );

			auto defer = ::q::make_shared< defer_type >( );

			defer->set_value( std::move( t ) );

			return defer->get_promise( );
		}
	}

private:
	// TODO: Make this lock-free and consider other list types
	mutex mutex_;
	std::queue< std::shared_ptr< defer_type > > waiters_;
	std::queue< tuple_type > queue_;
	std::atomic< bool > closed_;
};

} // namespace q

#endif // LIBQ_CHANNEL_HPP
