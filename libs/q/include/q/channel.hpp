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
struct channel_traits
{
	typedef q::promise< std::tuple< T... > > promise_type;
	typedef std::tuple< > promise_tuple_type;
	typedef q::arguments< > promise_arguments_type;
	typedef std::false_type is_shared;

	typedef std::tuple< T... > outer_tuple_type;
	typedef channel_traits< T... > type;
	typedef std::tuple< T... > tuple_type;
	typedef std::false_type is_promise;
};

template< typename... T >
struct channel_traits< q::promise< std::tuple< T... > > >
{
	typedef q::promise< std::tuple< T... > > promise_type;
	typedef std::tuple< T... > promise_tuple_type;
	typedef arguments< T... > promise_arguments_type;
	typedef std::false_type is_shared;

	typedef std::tuple< promise_type > outer_tuple_type;
	typedef channel_traits< promise_type > type;
	typedef std::tuple< promise_type > tuple_type;
	typedef std::true_type is_promise;
};

template< typename... T >
struct channel_traits< q::shared_promise< std::tuple< T... > > >
{
	typedef q::shared_promise< std::tuple< T... > > promise_type;
	typedef std::tuple< T... > promise_tuple_type;
	typedef arguments< T... > promise_arguments_type;
	typedef std::true_type is_shared;

	typedef std::tuple< promise_type > outer_tuple_type;
	typedef channel_traits< promise_type > type;
	typedef std::tuple< promise_type > tuple_type;
	typedef std::true_type is_promise;
};

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
	, close_exception_( )
	, closed_( false )
	, paused_( false )
	, buffer_count_( buffer_count )
	, resume_count_( std::min( resume_count, buffer_count ) )
	{ }

	std::size_t buffer_count( ) const
	{
		return buffer_count_;
	}

	bool is_closed( ) const
	{
		return closed_;
	}

	std::exception_ptr get_exception( ) const
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		return close_exception_;
	}

	void close( )
	{
		close( std::make_exception_ptr( channel_closed_exception( ) ) );
	}

	template< typename E >
	typename std::enable_if<
		!std::is_same<
			typename std::decay< E >::type,
			std::exception_ptr
		>::value
	>::type
	close( E&& e )
	{
		close( std::make_exception_ptr( std::forward< E >( e ) ) );
	}

	template< typename E >
	typename std::enable_if<
		std::is_same<
			typename std::decay< E >::type,
			std::exception_ptr
		>::value
		and
		!channel_traits< T... >::is_promise::value
	>::type
	close( E&& e )
	{
		close_sync( std::forward< E >( e ) );
	}

	template< typename E >
	typename std::enable_if<
		std::is_same<
			typename std::decay< E >::type,
			std::exception_ptr
		>::value
		and
		channel_traits< T... >::is_promise::value
	>::type
	close( E&& e )
	{
		// In case we treat async errors as closing the channel, which
		// is the case of promise-valued channels, we defer this
		// closing as the last thing to push on the queue.
		// This will close the channel when that promise is awaited.
		push_rejection( std::forward< E >( e ) );
	}

	template< typename E >
	typename std::enable_if<
		std::is_same<
			typename std::decay< E >::type,
			std::exception_ptr
		>::value
	>::type
	close_sync( E&& e )
	{
		task notification;

		{
			Q_AUTO_UNIQUE_LOCK( mutex_ );

			bool was_closed = closed_.exchange(
				true, std::memory_order_seq_cst );

			if ( !was_closed )
				close_exception_ = std::forward< E >( e );

			for ( auto& waiter : waiters_ )
				waiter->set_exception( close_exception_ );

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
			std::rethrow_exception( close_exception_ );

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
					default_queue_, close_exception_ );

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

	inline bool should_send( ) const
	{
		return !paused_ && !closed_;
	}

	void set_resume_notification( task fn )
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		resume_notification_ = fn;
	}

	void unset_resume_notification( )
	{
		set_resume_notification( task( ) );
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

	queue_ptr get_queue( ) const
	{
		return default_queue_;
	}

	// This will be called if this is a promise-valued channel with
	// stop-on-async-errors enabled.
	void clear( )
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		while ( !queue_.empty( ) )
			queue_.pop( );
	}

private:
	template< typename E >
	inline typename std::enable_if<
		std::is_same<
			typename std::decay< E >::type,
			std::exception_ptr
		>::value
		and
		!channel_traits< T... >::is_shared::value
	>::type
	push_rejection( E&& e )
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		typedef typename channel_traits< T... >::promise_arguments_type
			arguments_type;

		if ( closed_.load( std::memory_order_seq_cst ) )
			// Ignore this rejection/closing as we're already closed
			return;

		if ( waiters_.empty( ) )
		{
			auto promise = q::reject< arguments_type >(
				default_queue_, std::forward< E >( e ) );

			queue_.push( std::move( promise ) );
		}
		else
		{
			bool was_closed = closed_.exchange(
				true, std::memory_order_seq_cst );

			if ( !was_closed )
				close_exception_ = std::forward< E >( e );

			auto waiter = std::move( waiters_.front( ) );
			waiters_.pop_front( );

			waiter->set_exception( close_exception_ );
		}
	}

	template< typename E >
	inline typename std::enable_if<
		std::is_same<
			typename std::decay< E >::type,
			std::exception_ptr
		>::value
		and
		channel_traits< T... >::is_shared::value
	>::type
	push_rejection( E&& e )
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		typedef typename channel_traits< T... >::promise_arguments_type
			arguments_type;

		if ( closed_.load( std::memory_order_seq_cst ) )
			// Ignore this rejection/closing as we're already closed
			return;

		if ( waiters_.empty( ) )
		{
			auto promise = q::reject< arguments_type >(
				default_queue_, std::forward< E >( e ) );

			queue_.push( promise.share( ) );
		}
		else
		{
			bool was_closed = closed_.exchange(
				true, std::memory_order_seq_cst );

			if ( !was_closed )
				close_exception_ = std::forward< E >( e );

			auto waiter = std::move( waiters_.front( ) );
			waiters_.pop_front( );

			waiter->set_exception( close_exception_ );
		}
	}

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
	mutable mutex mutex_;
	std::list< std::shared_ptr< defer_type > > waiters_;
	std::queue< tuple_type > queue_;
	std::exception_ptr close_exception_;
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
	typedef typename detail::channel_traits< T... >::type traits;

	readable( ) = default;
	readable( const readable& ) = default;
	readable( readable&& ) = default;

	readable& operator=( const readable& ) = default;
	readable& operator=( readable&& ) = default;

	template< bool IsPromise = traits::is_promise::value >
	typename std::enable_if<
		!IsPromise,
		typename traits::promise_type
	>::type
	receive( )
	{
		return shared_channel_->receive( );
	}

	template< bool IsPromise = traits::is_promise::value >
	typename std::enable_if<
		IsPromise,
		typename traits::promise_type
	>::type
	receive( )
	{
		auto shared_channel = shared_channel_;

		return maybe_share( shared_channel_->receive( )
		.then( [ shared_channel ](
			typename traits::promise_type&& promise
		)
		{
			return promise
			.fail( [ shared_channel ]( std::exception_ptr e )
			-> typename traits::promise_type
			{
				shared_channel->close_sync( e );
				shared_channel->clear( );
				std::rethrow_exception(
					shared_channel->get_exception( ) );
			} );
		} ) );
	}

	std::size_t buffer_count( ) const
	{
		return shared_channel_->buffer_count( );
	}

	bool is_closed( ) const
	{
		return shared_channel_->is_closed( );
	}

	std::exception_ptr get_exception( ) const
	{
		return shared_channel_->get_exception( );
	}

	void close( )
	{
		shared_channel_->close( );
	}

	// Existence of this equals that of the shared_channel
	template< typename E >
	void close( E&& e )
	{
		shared_channel_->close( std::forward< E >( e ) );
	}

	void add_scope_until_closed( scope&& scope )
	{
		shared_channel_->add_scope_until_closed( std::move( scope ) );
	}

	queue_ptr get_queue( ) const
	{
		return shared_channel_->get_queue( );
	}

	/**
	 * Clears this channel from any queued values. The channel can still be
	 * used for future values.
	 *
	 * NOTE: This will not trigger back pressure notifications! Use `clear`
	 *       with caution, as you can easily end up with a stuck channel.
	 *       Most likely you do not want to use this function to begin
	 *       with, as it is a grand anti-pattern, only useful in rare
	 *       situations.
	 */
	void clear( )
	{
		shared_channel_->clear( );
	}

private:
	readable( std::shared_ptr< detail::shared_channel< T... > > ch )
	: shared_channel_( ch )
	, shared_owner_(
		std::make_shared< detail::shared_channel_owner< T... > >( ch ) )
	{ }

	template< typename Promise, bool Shared = traits::is_shared::value >
	typename std::enable_if<
		!Shared,
		typename traits::promise_type
	>::type
	maybe_share( Promise&& promise )
	{
		return std::forward< Promise >( promise );
	}

	template< typename Promise, bool Shared = traits::is_shared::value >
	typename std::enable_if<
		Shared,
		typename traits::promise_type
	>::type
	maybe_share( Promise&& promise )
	{
		return promise.share( );
	}

	friend class channel< T... >;

	std::shared_ptr< detail::shared_channel< T... > > shared_channel_;
	std::shared_ptr< detail::shared_channel_owner< T... > > shared_owner_;
};

template< typename... T >
class writable
{
public:
	typedef typename detail::channel_traits< T... >::type traits;
	typedef typename traits::tuple_type tuple_type;
	typedef typename traits::is_promise is_promise;
	typedef typename traits::promise_type promise_type;
	typedef typename traits::promise_tuple_type promise_tuple_type;
	typedef typename traits::outer_tuple_type outer_tuple_type;
	typedef typename traits::promise_arguments_type promise_arguments_type;

	writable( ) = default;
	writable( const writable& ) = default;
	writable( writable&& ) = default;

	writable& operator=( const writable& ) = default;
	writable& operator=( writable&& ) = default;

	/**
	 * ( tuple< T... > ) ->
	 */
	template< typename Tuple >
	typename std::enable_if<
		q::is_tuple< typename std::decay< Tuple >::type >::value
		and
		tuple_arguments<
			typename std::decay< Tuple >::type
		>::template is_convertible_to<
			typename tuple_arguments< outer_tuple_type >::this_type
		>::value
	>::type
	send( Tuple&& t )
	{
		shared_channel_->send( std::forward< Tuple >( t ) );
	}

	/**
	 * ( tuple< void_t > ) -> tuple< > (stripped from void_t)
	 */
	template< typename Tuple >
	typename std::enable_if<
		q::is_tuple< typename std::decay< Tuple >::type >::value
		and
		std::is_same<
			typename std::decay< Tuple >::type,
			std::tuple< void_t >
		>::value
		and
		std::is_same<
			outer_tuple_type,
			std::tuple< >
		>::value
	>::type
	send( Tuple&& t )
	{
		send( std::make_tuple( ) );
	}

	/**
	 * Non-promise channel
	 * ( Args... ) -> tuple< T... >
	 */
	template< typename... Args >
	typename std::enable_if<
		arguments<
			typename std::decay< Args >::type...
		>::template is_convertible_to_incl_void<
			arguments< T... >
		>::value
	>::type
	send( Args&&... args )
	{
		send( std::make_tuple( std::forward< Args >( args )... ) );
	}

	/**
	 * Promise-channel:
	 * ( Args... ) -> tuple< T... >
	 */
	template< typename... Args >
	typename std::enable_if<
		is_promise::value
		and
		arguments<
			typename std::decay< Args >::type...
		>::template is_convertible_to_incl_void<
			promise_arguments_type
		>::value
	>::type
	send( Args&&... args )
	{
		send( std::make_tuple( std::forward< Args >( args )... ) );
	}

	/**
	 * Promise-channel:
	 * ( tuple< void_t > ) -> tuple< >
	 * (stripped from void_t)
	 */
	template< typename Tuple >
	typename std::enable_if<
		is_promise::value
		and
		std::is_same<
			typename std::decay< Tuple >::type,
			std::tuple< void_t >
		>::value
		and
		std::is_same<
			promise_arguments_type,
			q::arguments< >
		>::value
	>::type
	send( Tuple&& t )
	{
		send( std::make_tuple( ) );
	}

	/**
	 * Promise-channel:
	 * ( tuple< T... > ) -> tuple< promise< tuple< T... > > >
	 */
	template< typename Tuple >
	typename std::enable_if<
		is_promise::value
		and
		tuple_arguments< typename std::decay< Tuple >::type >
			::template is_convertible_to< promise_arguments_type >
			::value
	>::type
	send( Tuple&& t )
	{
		send( std::make_tuple( suitable_with(
			shared_channel_->get_queue( ),
			std::forward< Tuple >( t )
		) ) );
	}

	bool should_send( ) const
	{
		return shared_channel_->should_send( );
	}

	void set_resume_notification( task fn )
	{
		shared_channel_->set_resume_notification( std::move( fn ) );
	}

	void unset_resume_notification( )
	{
		shared_channel_->unset_resume_notification( );
	}

	bool is_closed( ) const
	{
		return shared_channel_->is_closed( );
	}

	std::exception_ptr get_exception( ) const
	{
		return shared_channel_->get_exception( );
	}

	void close( )
	{
		shared_channel_->close( );
	}

	// Existence of this equals that of the shared_channel
	template< typename E >
	void close( E&& e )
	{
		shared_channel_->close( std::forward< E >( e ) );
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

	template< typename Tuple, bool Shared = traits::is_shared::value >
	typename std::enable_if< !Shared, promise_type >::type
	suitable_with( const queue_ptr& queue, Tuple&& t )
	{
		return q::with( queue, std::forward< Tuple >( t ) );
	}

	template< typename Tuple, bool Shared = traits::is_shared::value >
	typename std::enable_if< Shared, promise_type >::type
	suitable_with( const queue_ptr& queue, Tuple&& t )
	{
		return q::with( queue, std::forward< Tuple >( t ) ).share( );
	}

	friend class channel< T... >;

	std::shared_ptr< detail::shared_channel< T... > > shared_channel_;
	std::shared_ptr< detail::shared_channel_owner< T... > > shared_owner_;
};

template< typename... T >
class channel
{
public:
	typedef typename detail::channel_traits< T... > traits;

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
