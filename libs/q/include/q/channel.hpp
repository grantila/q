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
	typedef std::tuple< T... >     tuple_type;
	typedef detail::defer< T... >  defer_type;
	typedef arguments< T... >      arguments_type;
	typedef shared_channel< T... > self_type;

	struct waiter_type
	{
		virtual ~waiter_type( ) { }

		virtual void set_closed( ) = 0;
		virtual void set_exception( std::exception_ptr ) = 0;
		virtual void set_value( tuple_type&& ) = 0;
	};

	struct defer_waiter_type
	: waiter_type
	{
		void set_closed( ) override
		{
			deferred->set_exception(
				std::make_exception_ptr(
					channel_closed_exception( ) ) );
		}
		void set_exception( std::exception_ptr e ) override
		{
			deferred->set_exception( std::move( e ) );
		}
		void set_value( tuple_type&& t ) override
		{
			deferred->set_value( std::move( t ) );
		}

		defer_waiter_type( std::shared_ptr< defer_type > deferred )
		: deferred( deferred )
		{ }

		std::shared_ptr< defer_type > deferred;
	};

	template< typename FnValue, typename FnClosed >
	struct fast_waiter_type
	: waiter_type
	{
		typedef detail::defer< > result_defer_type;

		void set_closed( ) override
		{
			deferred->set_by_fun( fn_closed );
		}
		void set_exception( std::exception_ptr e ) override
		{
			deferred->set_exception( e );
		}
		void set_value( tuple_type&& t ) override
		{
			auto ch = shared_channel.lock( );

			// If the channel still exists (which should always be
			// the case), we'll make a proxy promise which detects
			// exceptions to automatically close the channel.
			if ( ch )
			{
				auto proxy = ::q::make_shared<
					result_defer_type
				>( ch->default_queue_ );

				proxy->set_by_fun( fn_value, std::move( t ) );

				deferred->satisfy(
					proxy->get_promise( )
					// TODO: Consider implementing
					//       promise::tap_error() for
					//       std::exception_ptr's to
					//       minimize the number of rethrows
					.fail( [ ch ]( std::exception_ptr e )
					{
						ch->close( e );
						ch->clear( );
						std::rethrow_exception( e );
					} )
				);
			}
			else
				deferred->set_by_fun( fn_value, std::move( t ) );
		}

		template< typename _FnValue, typename _FnClosed >
		fast_waiter_type(
			_FnValue&& fn_value,
			_FnClosed&& fn_closed,
			std::shared_ptr< result_defer_type > deferred,
			std::weak_ptr< self_type > shared_channel
		)
		: fn_value( std::forward< _FnValue >( fn_value ) )
		, fn_closed( std::forward< _FnClosed >( fn_closed ) )
		, deferred( deferred )
		, shared_channel( shared_channel )
		{ }

		FnValue fn_value;
		FnClosed fn_closed;
		std::shared_ptr< result_defer_type > deferred;
		std::weak_ptr< self_type > shared_channel;
	};

	shared_channel(
		const queue_ptr& queue,
		std::size_t buffer_count,
		std::size_t resume_count
	)
	: default_queue_( queue )
	, mutex_( Q_HERE, "channel" )
	, close_exception_( std::make_tuple( false, std::exception_ptr( ) ) )
	, closed_( false )
	, paused_( false )
	, buffer_count_( buffer_count )
	, resume_count_( std::min( resume_count, buffer_count ) )
	{ }

	Q_NODISCARD
	std::size_t buffer_count( ) const
	{
		return buffer_count_;
	}

	Q_NODISCARD
	bool is_closed( ) const
	{
		return closed_;
	}

	Q_NODISCARD
	bool has_exception( ) const
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		return std::get< 0 >( close_exception_ );
	}

	Q_NODISCARD
	std::exception_ptr get_exception( ) const
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		return std::get< 1 >( close_exception_ );
	}

	void close( )
	{
		_close( std::make_tuple( false, std::exception_ptr( ) ) );
	}

	template< typename E >
	typename std::enable_if<
		!std::is_same<
			typename std::decay< E >::type,
			std::exception_ptr
		>::value
		and
		!std::is_same<
			typename std::decay< E >::type,
			channel_closed_exception
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
			channel_closed_exception
		>::value
	>::type
	close( E&& e )
	{
		_close( std::make_tuple( false, std::exception_ptr( ) ) );
	}

	template< typename E >
	typename std::enable_if<
		std::is_same<
			typename std::decay< E >::type,
			std::exception_ptr
		>::value
	>::type
	close( E&& e )
	{
		_close( std::make_tuple( true, std::forward< E >( e ) ) );
	}

	Q_NODISCARD
	bool send( tuple_type&& t )
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		if ( closed_.load( std::memory_order_seq_cst ) )
			return false;

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

		return true;
	}

	Q_NODISCARD
	bool send( const tuple_type& t )
	{
		return send( tuple_type( t ) );
	}

	Q_NODISCARD
	promise< tuple_type > receive( )
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		if ( queue_.empty( ) )
		{
			if ( closed_.load( std::memory_order_seq_cst ) )
				return reject< arguments_type >(
					default_queue_,
					std::get< 0 >( close_exception_ )
					? std::get< 1 >( close_exception_ )
					: std::make_exception_ptr(
						channel_closed_exception( ) )
				);

			auto defer = ::q::make_shared< defer_type >(
				default_queue_ );

			waiters_.push_back(
				::q::make_unique< defer_waiter_type >(
					defer ) );
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

	/**
	 * Fast receive version, which doesn't use exceptions for close events.
	 *
	 * fn_value  will be called when the next value is available, and the
	 *           channels doesn't get closed.
	 * fn_closed will be called if the channel is/gets closed (and doesn't
	 *           get a next value), and doesn't contain an error.
	 *
	 * If the channel has/gets an exception, or any of the fn_value or
	 * fn_closed functions throw exceptions, this means the channel is
	 * closed (but fn_closed will not be called, since the channel was
	 * closed with an error), the returned promise will contain this
	 * exception.
	 * Otherwise, the returned promise will resolve.
	 *
	 * NOTE: If the fn_value throws an exception (synchronously or
	 * asynchronously), not only will this exception be propagated to the
	 * returned promise, the channel will be closed with this exception!
	 */
	template< typename FnValue, typename FnClosed >
	Q_NODISCARD
	promise< std::tuple< > >
	receive( FnValue&& fn_value, FnClosed&& fn_closed )
	{
		Q_AUTO_UNIQUE_LOCK( mutex_ );

		typedef fast_waiter_type<
			typename std::decay< FnValue >::type,
			typename std::decay< FnClosed >::type
		> specific_waiter_type;
		typedef typename specific_waiter_type::result_defer_type
			specific_defer_type;

		if ( queue_.empty( ) )
		{
			if ( closed_.load( std::memory_order_seq_cst ) )
			{
				if ( std::get< 0 >( close_exception_ ) )
					// There was a real error
					return reject< arguments< > >(
						default_queue_,
						std::get< 1 >(
							close_exception_ ) );
				else
				{
					// Nicely closed
					auto self = this->shared_from_this( );

					return q::make_promise(
						default_queue_, fn_closed );
				}
			}

			auto defer = ::q::make_shared< specific_defer_type >(
				default_queue_ );

			waiters_.push_back(
				::q::make_unique< specific_waiter_type >(
					std::forward< FnValue >( fn_value ),
					std::forward< FnClosed >( fn_closed ),
					defer,
					this->shared_from_this( )
				)
			);
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

			auto defer = ::q::make_shared< specific_defer_type >(
				default_queue_ );

			specific_waiter_type waiter(
				std::forward< FnValue >( fn_value ),
				std::forward< FnClosed >( fn_closed ),
				defer,
				this->shared_from_this( )
			);

			waiter.set_value( std::move( t ) );

			return defer->get_promise( );
		}
	}

	Q_NODISCARD
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
	 * Triggers (calls) the resume notification callback, if the channel
	 * allows more data to be sent, but not if the channel is "full".
	 */
	void trigger_resume_notification( )
	{
		task notification;

		{
			Q_AUTO_UNIQUE_LOCK( mutex_ );

			if ( !should_send( ) )
				return;

			notification = resume_notification_;
		}

		if ( notification )
			notification( );
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

	Q_NODISCARD
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
	template< typename... > friend class ::q::readable;

	template< typename Tuple >
	void _close( Tuple&& tup, bool force_exception = false )
	{
		task notification;

		{
			Q_AUTO_UNIQUE_LOCK( mutex_ );

			bool was_closed = closed_.exchange(
				true, std::memory_order_seq_cst );

			// When the channel is a channel of promises, and the
			// channel is nicely closed, an inner promise that is
			// rejected should close the channel with an error.
			// In this case, we overwrite the non-error with this
			// error.
			bool has_exception = std::get< 0 >( close_exception_ );
			bool overwrite = !has_exception && force_exception;

			if ( !was_closed || overwrite )
				close_exception_ = std::forward< Tuple >( tup );

			for ( auto& waiter : waiters_ )
			{
				if ( std::get< 0 >( close_exception_ ) )
					waiter->set_exception( std::get< 1 >(
						close_exception_ ) );
				else
					waiter->set_closed( );
			}

			waiters_.clear( );

			scopes_.clear( );

			notification = resume_notification_;
		}

		if ( notification )
			notification( );
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
	std::list< std::unique_ptr< waiter_type > > waiters_;
	std::queue< tuple_type > queue_;
	// True if arbitrary exception, false if "closed exception"
	std::tuple< bool, std::exception_ptr > close_exception_;
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
				shared_channel->_close( std::make_tuple(
					true, e ), true );
				shared_channel->clear( );
				std::rethrow_exception(
					shared_channel->get_exception( ) );
			} );
		} ) );
	}

	template<
		typename FnValue,
		typename FnClosed,
		bool IsPromise = traits::is_promise::value
	>
	typename std::enable_if<
		!IsPromise,
		promise< std::tuple< > >
	>::type
	receive( FnValue&& fn_value, FnClosed&& fn_closed )
	{
		return shared_channel_->receive(
			std::forward< FnValue >( fn_value ),
			std::forward< FnClosed >( fn_closed )
		);
	}

	template<
		typename FnValue,
		typename FnClosed,
		bool IsPromise = traits::is_promise::value
	>
	typename std::enable_if<
		IsPromise,
		promise< std::tuple< > >
	>::type
	receive( FnValue&& fn_value, FnClosed&& fn_closed )
	{
		auto shared_channel = shared_channel_;

		Q_MAKE_MOVABLE( fn_value );

		auto on_value =
			[ shared_channel, Q_MOVABLE_MOVE( fn_value ) ]
			( typename traits::promise_type&& promise )
		mutable -> ::q::promise< std::tuple< > >
		{
			auto fn = Q_MOVABLE_CONSUME( fn_value );
			Q_MAKE_MOVABLE( fn );

			return promise
			.then( std::move( fn ) )
			.fail(
				[ shared_channel ]( std::exception_ptr e )
				mutable -> void
			{
				shared_channel->_close( std::make_tuple(
					true, e ), true );
				shared_channel->clear( );
				std::rethrow_exception(
					shared_channel->get_exception( ) );
			} );
		};
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
	Q_NODISCARD
	typename std::enable_if<
		q::is_tuple< typename std::decay< Tuple >::type >::value
		and
		tuple_arguments<
			typename std::decay< Tuple >::type
		>::template is_convertible_to<
			typename tuple_arguments< outer_tuple_type >::this_type
		>::value,
		bool
	>::type
	send( Tuple&& t )
	{
		return shared_channel_->send( std::forward< Tuple >( t ) );
	}

	/**
	 * ( tuple< void_t > ) -> tuple< > (stripped from void_t)
	 */
	template< typename Tuple >
	Q_NODISCARD
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
		>::value,
		bool
	>::type
	send( Tuple&& t )
	{
		return send( std::make_tuple( ) );
	}

	/**
	 * Non-promise channel
	 * ( Args... ) -> tuple< T... >
	 */
	template< typename... Args >
	Q_NODISCARD
	typename std::enable_if<
		arguments<
			typename std::decay< Args >::type...
		>::template is_convertible_to_incl_void<
			arguments< T... >
		>::value,
		bool
	>::type
	send( Args&&... args )
	{
		return send(
			std::make_tuple( std::forward< Args >( args )... ) );
	}

	/**
	 * Promise-channel:
	 * ( Args... ) -> tuple< T... >
	 */
	template< typename... Args >
	Q_NODISCARD
	typename std::enable_if<
		is_promise::value
		and
		arguments<
			typename std::decay< Args >::type...
		>::template is_convertible_to_incl_void<
			promise_arguments_type
		>::value,
		bool
	>::type
	send( Args&&... args )
	{
		return send(
			std::make_tuple( std::forward< Args >( args )... ) );
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
		>::value,
		bool
	>::type
	send( Tuple&& t )
	{
		return send( std::make_tuple( ) );
	}

	/**
	 * Promise-channel:
	 * ( tuple< T... > ) -> tuple< promise< tuple< T... > > >
	 */
	template< typename Tuple >
	Q_NODISCARD
	typename std::enable_if<
		is_promise::value
		and
		tuple_arguments< typename std::decay< Tuple >::type >
			::template is_convertible_to< promise_arguments_type >
			::value,
		bool
	>::type
	send( Tuple&& t )
	{
		return send( std::make_tuple( suitable_with(
			shared_channel_->get_queue( ),
			std::forward< Tuple >( t )
		) ) );
	}

	/**
	 * Like send() but throws q::channel_closed_exception if the channel
	 * was closed.
	 */
	template< typename... Any >
	void ensure_send( Any&&... t )
	{
		if ( !send( std::forward< Any >( t )... ) )
			Q_THROW( channel_closed_exception( ) );
	}

	Q_NODISCARD
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

	void trigger_resume_notification( )
	{
		shared_channel_->trigger_resume_notification( );
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
