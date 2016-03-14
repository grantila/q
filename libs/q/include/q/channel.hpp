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
#include <q/scope.hpp>

#include <list>
#include <queue>
#include <atomic>

namespace q {

Q_MAKE_SIMPLE_EXCEPTION( channel_closed_exception );

template< typename... T >
class readable;

template< typename... T >
using readable_ptr = std::shared_ptr< readable< T... > >;

template< typename... T >
class writable;

template< typename... T >
using writable_ptr = std::shared_ptr< writable< T... > >;

template< typename... T >
class channel;

template< typename... T >
using channel_ptr = std::shared_ptr< channel< T... > >;


template< typename... T >
class readable
{
public:
	typedef std::tuple< T... > tuple_type;

	virtual promise< tuple_type > receive( ) = 0;

	virtual bool is_closed( ) = 0;

	virtual void close( ) = 0;
};

template< typename... T >
class writable
{
public:
	typedef std::tuple< T... > tuple_type;

	template< typename... Args >
	typename std::enable_if<
		(
			sizeof...( Args ) != 1 ||
			!arguments<
				typename arguments< Args... >::first_type
			>::template is_convertible_to<
				arguments< tuple_type >
			>::value
		) &&
		arguments<
			Args...
		>::template is_convertible_to<
			typename tuple_arguments< tuple_type >::this_type
		>::value
	>::type
	send( Args&&... args )
	{
		this->send( std::forward_as_tuple( args... ) );
	}

	virtual void send( tuple_type&& t ) = 0;

	virtual bool should_send( ) = 0;

	virtual void set_resume_notification( task fn ) = 0;

	virtual bool is_closed( ) = 0;

	virtual void close( ) = 0;
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

	static constexpr std::size_t default_resume_count( std::size_t count )
	{
		return count < 3 ? count : ( ( count * 3 ) / 4 );
	}

	channel( const queue_ptr& queue, std::size_t buffer_count )
	: channel( queue, buffer_count, default_resume_count( buffer_count ) )
	{ }

	channel( const queue_ptr& queue, std::size_t buffer_count, std::size_t resume_count /* q::location, name */ )
	: default_queue_( queue )
	, mutex_( Q_HERE, "channel" )
	, closed_( false )
	, paused_( false )
	, buffer_count_( buffer_count )
	, resume_count_( resume_count )
	{ }

	bool is_closed( ) override
	{
		return closed_;
	}

	void close( ) override
	{
		task notification;

		{
			Q_AUTO_UNIQUE_LOCK( mutex_ );

			closed_.store( true, std::memory_order_seq_cst );

			for ( auto& waiter : waiters_ )
				waiter->set_exception(
					channel_closed_exception( ) );

			notification = resume_notification_;
		}

		if ( notification )
			notification( );
	}

	using writable< T... >::send;

	void send( tuple_type&& t ) override
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		if ( closed_.load( std::memory_order_seq_cst ) )
			Q_THROW( channel_closed_exception( ) );

		if ( waiters_.empty( ) )
		{
			if ( queue_.size( ) >= buffer_count_ )
				paused_ = true;

			queue_.push( std::move( t ) );
		}
		else
		{
			auto waiter = std::move( waiters_.front( ) );
			waiters_.pop_front( );

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
					default_queue_,
					channel_closed_exception( ) );

			auto defer = ::q::make_shared< defer_type >(
				default_queue_ );

			waiters_.push_back( defer );
			resume( );

			return defer->get_promise( );
		}
		else
		{
			tuple_type t = std::move( queue_.front( ) );
			queue_.pop( );

			if ( queue_.size( ) < resume_count_ )
			{
				auto self = this->shared_from_this( );
				default_queue_->push( [ self ]( )
				{
					self->resume( );
				} );
			}

			auto defer = ::q::make_shared< defer_type >(
				default_queue_ );

			defer->set_value( std::move( t ) );

			return defer->get_promise( );
		}
	}

	inline bool should_send( ) override
	{
		return !paused_ && !closed_;
	}

	void set_resume_notification( task fn ) override
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		resume_notification_ = fn;
	}

	/**
	 * Adds a scope to this channel. This will cause the channel to "own"
	 * the scope, and thereby destruct it when the channel is destructed.
	 */
	void add_scope( scope&& scope )
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		scopes_.emplace_back( std::move( scope ) );
	}

private:
	inline void resume( )
	{
		if ( paused_.exchange( false ) )
		{
			auto& trigger_resume = resume_notification_;
			if ( trigger_resume )
				trigger_resume( );
		}
	}

	queue_ptr default_queue_;
	// TODO: Make this lock-free and consider other list types
	mutex mutex_;
	std::list< std::shared_ptr< defer_type > > waiters_;
	std::queue< tuple_type > queue_;
	std::atomic< bool > closed_;
	std::atomic< bool > paused_;
	const std::size_t buffer_count_;
	const std::size_t resume_count_;
	task resume_notification_;
	std::vector< scope > scopes_;
};

} // namespace q

#endif // LIBQ_CHANNEL_HPP
