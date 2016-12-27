/*
 * Copyright 2016 Gustaf Räntilä
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

#ifndef LIBQ_RX_OBSERVABLE_TRANSFORMERS_GROUP_BY_HPP
#define LIBQ_RX_OBSERVABLE_TRANSFORMERS_GROUP_BY_HPP

namespace q { namespace rx {

namespace {

// TODO: Introduce an option for propagate close and/or error events upstream
//       from each branch, to the joint and then continue upstream/downstream
//       to the other branches. Sometimes it's useful to let a branch fail (or
//       close successively) without having this propagating to the other
//       branches and upstream.
//       This is applicable to tee() and some other operators too, and should
//       be a q::option typedef.

template<
	typename Key,
	typename Val,
	typename Operator,
	bool AsHash = detail::is_unary_operator< Operator >::value
>
struct container_of_operation
{
	typedef std::unordered_map< Key, Val, Operator > type;
};
template< typename Key, typename Val, typename Operator >
struct container_of_operation< Key, Val, Operator, false >
{
	typedef std::map< Key, Val, Operator > type;
};

template<
	typename T,
	typename key_type,
	typename Operator,
	typename fn_type,
	bool IsAsync = q::is_promise< q::result_of_t< fn_type > >::value
>
struct group_by_context
: public std::enable_shared_from_this<
	group_by_context< T, key_type, Operator, fn_type >
>
{
	typedef group_by_context< T, key_type, Operator, fn_type > this_type;

	typedef detail::observable_types< void > base_types;

	typedef typename q::objectify< T >::type void_safe_type;

	typedef q::channel< key_type, observable< T > > outer_channel_type;
	typedef base_types::channel_type inner_channel_type;

	typedef observable< std::tuple< key_type, observable< T > > >
		outer_observable_type;
	typedef observable< T > inner_observable;

	typedef q::writable< key_type, observable< T > > outer_writable_type;
	typedef q::writable< void_safe_type > inner_writable_type;

	struct group
	{
		key_type key;
		q::channel< void_safe_type > channel;
	};

	typedef typename container_of_operation<
		key_type, group, Operator
	>::type map_type;


	fn_type fn_;
	std::shared_ptr< this_type > self_ptr_;
	queue_ptr queue_;
	queue_ptr next_queue_;
	std::size_t inner_backlog_size_;
	std::size_t outer_backlog_size_;
	outer_writable_type outer_writable_;
	map_type groups_;


	group_by_context(
		fn_type&& fn,
		queue_ptr&& queue,
		queue_ptr&& next_queue,
		std::size_t backlog_size,
		q::concurrency concurrency
	)
	: fn_( std::move( fn ) )
	, queue_( std::move( queue ) )
	, next_queue_( std::move( next_queue ) )
	, outer_backlog_size_( backlog_size )
	{
		// TODO: Make this configurable independently of the outer size
		inner_backlog_size_ = outer_backlog_size_;

		// TODO: Implement concurrency
		q::unused_variable( concurrency );
	}

	outer_observable_type setup( observable< T >&& source )
	{
		auto self = this->shared_from_this( );
		self_ptr_ = self;

		outer_channel_type channel_(
			std::move( next_queue_ ), outer_backlog_size_ );

		auto writable = channel_.get_writable( );

		outer_writable_ = writable;

		source.consume( [ self ]( void_safe_type&& t )
		{
			return self->on_data( std::move( t ) );
		} )
		.fail( [ writable ]( std::exception_ptr e ) mutable
		{
			// Any error on the input observable is immediately forwarded.
			// This is how all operators are expected to work.
			writable.close( e );
		} )
		.finally( [ self ]( )
		{
			self->cleanup( );
		} );

		return outer_observable_type( channel_ );
	}

	void abort( std::exception_ptr err )
	{
		// TODO: Signal error upstream
		// Signal error downstream
		outer_writable_.close( err );

		outer_writable_.unset_resume_notification( );

		for ( auto& iter : groups_ )
		{
			auto& channel = iter.second.channel;
			auto writable = channel.get_writable( );
			writable.unset_resume_notification( );
		}
	}

	void cleanup( )
	{
		// TODO: Close upstream
		// Close downstream
		outer_writable_.close( );

		outer_writable_.unset_resume_notification( );

		for ( auto& iter : groups_ )
		{
			auto& channel = iter.second.channel;
			auto writable = channel.get_writable( );
			writable.unset_resume_notification( );
			writable.close( );
		}

		// Remove self-reference to allow deallocation
		self_ptr_.reset( );
	}

	q::promise< >
	send( key_type&& key, void_safe_type&& value )
	{
		inner_writable_type writable;

		auto iter = groups_.find( key );

		std::shared_ptr< backpressure > backp;

		if ( iter != groups_.end( ) )
		{
			writable = iter->second.channel.get_writable( );
		}
		else
		{
			std::tie( iter, std::ignore ) =
				groups_.emplace( key, group{
					key,
					{ queue_, inner_backlog_size_ }
				} );

			auto& group = iter->second;
			auto& channel = group.channel;

			writable = channel.get_writable( );

			ignore_result( outer_writable_.send(
				key, inner_observable( channel ) ) );

			if ( !outer_writable_.should_send( ) )
			{
				backp = q::make_shared< backpressure >(
					queue_ );

				backp->add_once( outer_writable_ );
			}
		}

		if ( writable.is_closed( ) )
		{
			if ( !writable.get_exception( ) )
				// A closed inner observable means we
				// just drop those items.
				return q::with( queue_ );
			else
			{
				// Propagate downstream errors upstream
				abort( writable.get_exception( ) );
				return q::with( queue_ );
			}
		}

		ignore_result( writable.send( std::move( value ) ) );

		if ( !writable.should_send( ) )
		{
			if ( !backp )
				backp = q::make_shared< backpressure >(
					queue_ );

			backp->add_once( writable );
		}

		if ( !backp )
			return q::with( queue_ );

		return backp->get_promise( );
	}

	template< bool _IsAsync = IsAsync >
	typename std::enable_if< !_IsAsync, q::promise< > >::type
	on_data( void_safe_type&& value )
	{
		auto self = self_ptr_;

		return q::with( queue_ )
		.then( [ self, value{ std::move( value ) } ]( ) mutable
		{
			key_type key = self->fn_( value );

			return self->send(
				std::move( key ), std::move( value ) );
		} )
		.tap_error( [ self ]( std::exception_ptr e )
		{
			// Propagate error downstream, by failing (rethrowing)
			// in on_data, this will cause upstream consumption to
			// fail too, which is what we want.
			self->abort( e );
		} );
	}

	template< bool _IsAsync = IsAsync >
	typename std::enable_if< _IsAsync, q::promise< > >::type
	on_data( void_safe_type&& value )
	{
		auto self = self_ptr_;

		auto shared_value =
			std::make_shared< void_safe_type >(
				std::move( value ) );

		return q::with( queue_ )
		.then( [ self, shared_value{ std::move( shared_value ) } ]( )
		{
			return self->fn_( *shared_value )
			.then( [ self, shared_value ]( key_type key )
			{
				return self->send(
					std::move( key ),
					std::move( *shared_value )
				);
			} );
		} )
		.tap_error( [ self ]( std::exception_ptr e )
		{
			// Propagate error downstream, by failing (rethrowing)
			// in on_data, this will cause upstream consumption to
			// fail too, which is what we want.
			self->abort( e );
		} );
	}
};

} // anonymous namespace

template< typename T >
template< typename Operator, typename Fn >
std::enable_if_t<
	should_hash_v<
		std::decay_t< q::result_of_t< Fn > >,
		Operator
	>
	and
	!q::is_promise< q::result_of_t< Fn > >::value
	and
	!q::result_of_as_argument_t< Fn >::empty_or_voidish::value,
	observable< std::tuple<
		std::decay_t< q::result_of_t< Fn > >,
		observable< T >
	> >
>
observable< T >::
group_by( Fn&& fn, work_options options )
{
	typedef std::decay_t< q::result_of_t< Fn > > key_type;
	typedef typename specific_hasher< key_type, Operator >::type hash_type;
	typedef std::decay_t< Fn > fn_type;

	typedef group_by_context<
		T,
		key_type,
		hash_type,
		fn_type
	> context_type;

	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( readable_->get_queue( ) ) ).value;
	auto queue = options.get< q::queue_ptr >( next_queue );
	auto backlog_size = options.get< backlog >( Q_RX_DEFAULT_BACKLOG );
	auto concurrency = options.get< q::concurrency >( );

	auto ctx = std::make_shared< context_type >(
		std::is_lvalue_reference< Fn >::value
			? fn_type( fn )
			: std::forward< Fn >( fn ),
		std::move( queue ),
		std::move( next_queue ),
		std::move( backlog_size ),
		std::move( concurrency )
	);

	return ctx->setup( std::move( *this ) );
}

template< typename T >
template< typename Operator, typename Fn >
std::enable_if_t<
	!should_hash_v<
		std::decay_t< q::result_of_t< Fn > >,
		Operator
	>
	and
	!q::is_promise< q::result_of_t< Fn > >::value
	and
	!q::result_of_as_argument_t< Fn >::empty_or_voidish::value,
	observable< std::tuple<
		std::decay_t< q::result_of_t< Fn > >,
		observable< T >
	> >
>
observable< T >::
group_by( Fn&& fn, work_options options )
{
	typedef std::decay_t< q::result_of_t< Fn > > key_type;
	typedef typename specific_less< key_type, Operator >::type less_type;
	typedef std::decay_t< Fn > fn_type;

	typedef group_by_context<
		T,
		key_type,
		less_type,
		fn_type
	> context_type;

	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( readable_->get_queue( ) ) ).value;
	auto queue = options.get< q::queue_ptr >( next_queue );
	auto backlog_size = options.get< backlog >( Q_RX_DEFAULT_BACKLOG );
	auto concurrency = options.get< q::concurrency >( );

	auto ctx = std::make_shared< context_type >(
		std::is_lvalue_reference< Fn >::value
			? fn_type( fn )
			: std::forward< Fn >( fn ),
		std::move( queue ),
		std::move( next_queue ),
		std::move( backlog_size ),
		std::move( concurrency )
	);

	return ctx->setup( std::move( *this ) );
}

template< typename T >
template< typename Operator, typename Fn >
std::enable_if_t<
	should_hash_v<
		std::decay_t<
			typename q::result_of_t< Fn >::first_or_all_types
		>,
		Operator
	>
	and
	q::is_promise< q::result_of_t< Fn > >::value
	and
	!q::result_of_t< Fn >::argument_types::empty_or_voidish::value,
	observable< std::tuple<
		std::decay_t<
			typename q::result_of_t< Fn >::first_or_all_types
		>,
		observable< T >
	> >
>
observable< T >::
group_by( Fn&& fn, work_options options )
{
	typedef std::decay_t<
		typename q::result_of_t< Fn >::first_or_all_types
	> key_type;
	typedef typename specific_hasher< key_type, Operator >::type hash_type;
	typedef std::decay_t< Fn > fn_type;

	typedef group_by_context<
		T,
		key_type,
		hash_type,
		fn_type
	> context_type;

	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( readable_->get_queue( ) ) ).value;
	auto queue = options.get< q::queue_ptr >( next_queue );
	auto backlog_size = options.get< backlog >( Q_RX_DEFAULT_BACKLOG );
	auto concurrency = options.get< q::concurrency >( );

	auto ctx = std::make_shared< context_type >(
		std::is_lvalue_reference< Fn >::value
			? fn_type( fn )
			: std::forward< Fn >( fn ),
		std::move( queue ),
		std::move( next_queue ),
		std::move( backlog_size ),
		std::move( concurrency )
	);

	return ctx->setup( std::move( *this ) );
}

template< typename T >
template< typename Operator, typename Fn >
std::enable_if_t<
	!should_hash_v<
		std::decay_t<
			typename q::result_of_t< Fn >::first_or_all_types
		>,
		Operator
	>
	and
	q::is_promise< q::result_of_t< Fn > >::value
	and
	!q::result_of_t< Fn >::argument_types::empty_or_voidish::value,
	observable< std::tuple<
		std::decay_t<
			typename q::result_of_t< Fn >::first_or_all_types
		>,
		observable< T >
	> >
>
observable< T >::
group_by( Fn&& fn, work_options options )
{
	typedef std::decay_t<
		typename q::result_of_t< Fn >::first_or_all_types
	> key_type;
	typedef typename specific_less< key_type, Operator >::type less_type;
	typedef std::decay_t< Fn > fn_type;

	typedef group_by_context<
		T,
		key_type,
		less_type,
		fn_type
	> context_type;

	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( readable_->get_queue( ) ) ).value;
	auto queue = options.get< q::queue_ptr >( next_queue );
	auto backlog_size = options.get< backlog >( Q_RX_DEFAULT_BACKLOG );
	auto concurrency = options.get< q::concurrency >( );

	auto ctx = std::make_shared< context_type >(
		std::is_lvalue_reference< Fn >::value
			? fn_type( fn )
			: std::forward< Fn >( fn ),
		std::move( queue ),
		std::move( next_queue ),
		std::move( backlog_size ),
		std::move( concurrency )
	);

	return ctx->setup( std::move( *this ) );
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_TRANSFORMERS_GROUP_BY_HPP
