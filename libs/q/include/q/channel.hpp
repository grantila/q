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
class writable;

template< typename... T >
class channel;

namespace detail {

static constexpr std::size_t default_resume_count( std::size_t count )
{
	return count < 3 ? count : ( ( count * 3 ) / 4 );
}

template< typename... T >
class shared_channel
: public std::enable_shared_from_this< shared_channel< T... > >
{
public:
	typedef std::tuple< T... >    tuple_type;
	typedef detail::defer< T... > defer_type;
	typedef arguments< T... >     arguments_type;

	shared_channel(
		const queue_ptr& queue,
		std::size_t buffer_count,
		std::size_t resume_count
	)
	: default_queue_( queue )
	, mutex_( Q_HERE, "channel" )
	, closed_( false )
	, paused_( false )
	, buffer_count_( buffer_count )
	, resume_count_( std::min( resume_count, buffer_count ) )
	{ }

	bool is_closed( )
	{
		return closed_;
	}

	void close( )
	{
		task notification;

		{
			Q_AUTO_UNIQUE_LOCK( mutex_ );

			closed_.store( true, std::memory_order_seq_cst );

			for ( auto& waiter : waiters_ )
				waiter->set_exception(
					channel_closed_exception( ) );

			waiters_.clear( );

			scopes_.clear( );

			notification = resume_notification_;
		}

		if ( notification )
			notification( );
	}

	void send( tuple_type&& t )
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

	promise< tuple_type > receive( )
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

	inline bool should_send( )
	{
		return !paused_ && !closed_;
	}

	void set_resume_notification( task fn )
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		resume_notification_ = fn;
	}

	/**
	 * Adds a scope to this channel. This will cause the channel to "own"
	 * the scope, and thereby destruct it when the channel is destructed.
	 */
	void add_scope_until_closed( scope&& scope )
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		if ( closed_ )
			// Already closed - don't keep scope
			return;

		scopes_.emplace_back( std::move( scope ) );
	}

	queue_ptr get_queue( )
	{
		return default_queue_;
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

template< typename... T >
class shared_channel_owner
{
public:
	shared_channel_owner( ) = delete;
	shared_channel_owner( const shared_channel_owner& ) = default;
	shared_channel_owner( shared_channel_owner&& ) = default;

	shared_channel_owner& operator=( const shared_channel_owner& ) = default;
	shared_channel_owner& operator=( shared_channel_owner&& ) = default;

	shared_channel_owner(
		std::shared_ptr< detail::shared_channel< T... > > ch )
	: shared_channel_( std::move( ch ) )
	{ }

	~shared_channel_owner( )
	{
		shared_channel_->close( );
	}

protected:
	std::shared_ptr< detail::shared_channel< T... > > shared_channel_;
};

} // namespace detail

template< typename... T >
class readable
{
public:
	typedef std::tuple< T... > tuple_type;

	readable( ) = default;
	readable( const readable& ) = default;
	readable( readable&& ) = default;

	readable& operator=( const readable& ) = default;
	readable& operator=( readable&& ) = default;

	promise< tuple_type > receive( )
	{
		return shared_channel_->receive( );
	}

	bool is_closed( )
	{
		return shared_channel_->is_closed( );
	}

	void close( )
	{
		shared_channel_->close( );
	}

	void add_scope_until_closed( scope&& scope )
	{
		shared_channel_->add_scope_until_closed( std::move( scope ) );
	}

	queue_ptr get_queue( )
	{
		return shared_channel_->get_queue( );
	}

private:
	readable( std::shared_ptr< detail::shared_channel< T... > > ch )
	: shared_channel_( ch )
	, shared_owner_(
		std::make_shared< detail::shared_channel_owner< T... > >( ch ) )
	{ }

	friend class channel< T... >;

	std::shared_ptr< detail::shared_channel< T... > > shared_channel_;
	std::shared_ptr< detail::shared_channel_owner< T... > > shared_owner_;
};

template< typename... T >
class writable
{
public:
	typedef std::tuple< T... > tuple_type;

	writable( ) = default;
	writable( const writable& ) = default;
	writable( writable&& ) = default;

	writable& operator=( const writable& ) = default;
	writable& operator=( writable&& ) = default;

	template< typename... Args >
	typename std::enable_if<
		(
			sizeof...( Args ) != 1
			or
			arguments<
				typename arguments< Args... >::first_type
			>::template is_convertible_to<
				typename tuple_arguments< tuple_type >::this_type
			>::value
		)
		and
		( sizeof...( Args ) > 0 )
		and
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

	template< typename T_ = tuple_type >
	typename std::enable_if<
		is_empty_tuple< T_ >::value
	>::type
	send( )
	{
		this->send( std::make_tuple( ) );
	}

	template< typename Tuple >
	typename std::enable_if<
		q::is_tuple< typename std::decay< Tuple >::type >::value
		and
		q::tuple_convertible_to_tuple<
			typename std::decay< Tuple >::type,
			tuple_type
		>::value
	>::type
	send( Tuple&& t )
	{
		shared_channel_->send( std::forward< Tuple >( t ) );
	}

	bool should_send( )
	{
		return shared_channel_->should_send( );
	}

	void set_resume_notification( task fn )
	{
		shared_channel_->set_resume_notification( std::move( fn ) );
	}

	bool is_closed( )
	{
		return shared_channel_->is_closed( );
	}

	void close( )
	{
		shared_channel_->close( );
	}

	void add_scope_until_closed( scope&& scope )
	{
		shared_channel_->add_scope_until_closed( std::move( scope ) );
	}

private:
	writable( std::shared_ptr< detail::shared_channel< T... > > ch )
	: shared_channel_( ch )
	, shared_owner_(
		std::make_shared< detail::shared_channel_owner< T... > >( ch ) )
	{ }

	friend class channel< T... >;

	std::shared_ptr< detail::shared_channel< T... > > shared_channel_;
	std::shared_ptr< detail::shared_channel_owner< T... > > shared_owner_;
};

template< typename... T >
class channel
{
public:
	channel( const queue_ptr& queue, std::size_t buffer_count )
	: channel(
		queue,
		buffer_count,
		detail::default_resume_count( buffer_count )
	)
	{ }

	channel(
		const queue_ptr& queue,
		std::size_t buffer_count,
		std::size_t resume_count
	)
	: shared_channel_(
		q::make_shared< detail::shared_channel< T... > >(
			queue, buffer_count, resume_count ) )
	, readable_( shared_channel_ )
	, writable_( shared_channel_ )
	{ }

	readable< T... > get_readable( )
	{
		return readable_;
	}

	writable< T... > get_writable( )
	{
		return writable_;
	}

	void add_scope_until_closed( scope&& scope )
	{
		shared_channel_->add_scope_until_closed( std::move( scope ) );
	}

private:
	std::shared_ptr< detail::shared_channel< T... > > shared_channel_;
	readable< T... > readable_;
	writable< T... > writable_;
};

} // namespace q

#endif // LIBQ_CHANNEL_HPP
