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

#ifndef LIBQ_RX_OBSERVABLE_CONSUMERS_CONSUME_HPP
#define LIBQ_RX_OBSERVABLE_CONSUMERS_CONSUME_HPP

namespace q { namespace rx {

namespace detail {

template< typename fn_type, typename readable_type >
struct consume_context
: public std::enable_shared_from_this<
	consume_context< fn_type, readable_type >
>
{
	readable_type readable;
	std::unique_ptr< fn_type > fn;
	q::queue_ptr queue;
	q::queue_ptr next_queue;
	std::size_t max_concurrency;
	std::atomic< std::size_t > concurrency;
	std::shared_ptr< std::vector< q::promise< std::tuple< > > > >
		concurrent_readers;
	q::resolver< > resolver;
	q::rejecter< > rejecter;

	consume_context( readable_type readable, base_options&& options )
	: readable( readable )
	, concurrency( 0 )
	{
		next_queue = options.get<
			q::defaultable< q::queue_ptr >
		>( q::set_default( readable->get_queue( ) ) ).value;
		auto concurrency = options.get<
			q::concurrency
		>( 1 );

		queue = options.get< q::queue_ptr >( next_queue );

		max_concurrency = concurrency.get( );
	}

	void work( q::resolver< > resolve, q::rejecter< > reject )
	{
		auto self = this->shared_from_this( );

		struct worker_type
		{
			std::function< void( void ) >
				recursive_consumer;
			q::resolver< > resolve;
			q::rejecter< > reject;
		};

		auto worker = std::make_shared< worker_type >( );

		worker->resolve = std::move( resolve );
		worker->reject = std::move( reject );

		worker->recursive_consumer = [ self, worker ]( )
		{
			auto fn = *self->fn;
			self->readable->receive( std::move( fn ), self->queue )
			.then( [ worker ]( )
			{
				// No error, there might be
				// more data to read
				worker->recursive_consumer( );
			} )
			.fail( [ worker ](
				const q::channel_closed_exception&
			)
			{
				// Channel closed successfully
				worker->recursive_consumer = nullptr;
				worker->resolve( );
			} )
			.fail( [ worker ](
				std::exception_ptr e
			)
			{
				// Channel closed with error
				worker->recursive_consumer = nullptr;
				worker->reject( e );
			} );
		};

		worker->recursive_consumer( );
	}

	static void consumer(
		std::shared_ptr< consume_context > self,
		q::resolver< > resolve,
		q::rejecter< > reject
	)
	{
		self->resolver = resolve;
		self->rejecter = reject;

		self->concurrent_readers = std::make_shared<
			std::vector< q::promise< std::tuple< > > >
		>( );

		while ( self->concurrency < self->max_concurrency )
		{
			self->concurrency += 1;

			auto worker = [ self ](
				q::resolver< > resolve,
				q::rejecter< > reject
			)
			{
				self->work( resolve, reject );
			};

			self->concurrent_readers->push_back(
				q::make_promise_sync(
					self->queue, std::move( worker ) )
			);
		}

		q::all( *self->concurrent_readers, self->queue )
		.strip( )
		.then( [ self ]( )
		{
			auto exception = self->readable->get_exception( );
			if ( exception )
				std::rethrow_exception( exception );

			// This should likely never happen
			self->resolver( );
		} )
		.fail( [ self ]( q::channel_closed_exception e )
		{
			self->resolver( );
		} )
		.fail( [ self ](
			const q::combined_promise_exception< void >& e
		)
		{
			if ( e.exceptions( ).empty( ) )
				// This should never happen
				self->rejecter( e );
			else
				self->rejecter( e.exceptions( )[ 0 ] );
		} )
		.fail( [ self ]( std::exception_ptr e )
		{
			// This should never happen
			self->rejecter( e );
		} );
	}

	::q::promise< std::tuple< > >
	consume( )
	{
		auto self = this->shared_from_this( );

		typedef std::function<
			void( q::resolver< >, q::rejecter< > )
		> fun_type;

		fun_type consumer = std::bind(
			&consume_context::consumer,
			self,
			std::placeholders::_1,
			std::placeholders::_2 );

		return q::make_promise_sync( queue, consumer )
		.use_queue( next_queue );
	}
};

} // namespace detail

/**
 * ( T ) -> void
 */
template< typename T >
template< typename Fn >
typename std::enable_if<
	(
		(
			arguments_of_are_convertible_from_incl_void_v< Fn, T >
			and
			q::arity_of_v< Fn > != 0
		)
		or
		(
			is_tuple_v< T >
			and
			tuple_arguments_t< T >
				::template is_convertible_to_incl_void<
					arguments_of_t< Fn >
				>::value
		)
	)
	and
	std::is_void< ::q::result_of_t< Fn > >::value,
	::q::promise< std::tuple< > >
>::type
observable< T >::
consume( Fn&& fn, base_options options )
{
	typedef std::shared_ptr< detail::observable_readable< T > >
		readable_type;
	typedef decayed_function_t< Fn > fn_type;

	typedef detail::consume_context< fn_type, readable_type > context_type;

	auto context = std::make_shared< context_type >(
		readable_, std::move( options ) );

	context->fn = q::make_unique< fn_type >( std::forward< Fn >( fn ) );

	return context->consume( );
}

template< typename T >
template< typename Fn >
typename std::enable_if<
	(
		(
			arguments_of_are_convertible_from_incl_void_v< Fn, T >
			and
			q::arity_of_v< Fn > != 0
		)
		or
		(
			is_tuple_v< T >
			and
			tuple_arguments_t< T >
				::template is_convertible_to_incl_void<
					arguments_of_t< Fn >
				>::value
		)
	)
	and
	::q::is_promise_v< std::decay_t< ::q::result_of_t< Fn > > >
	and
	::q::result_of_t< Fn >::argument_types::empty_v,
	::q::promise< std::tuple< > >
>::type
observable< T >::
consume( Fn&& fn, base_options options )
{
/*
	// TODO: Start using the generic consumption helper:

	typedef std::shared_ptr< detail::observable_readable< T > >
		readable_type;
	typedef decayed_function_t< Fn > fn_type;

	typedef detail::consume_context< fn_type, readable_type > context_type;

	auto context = std::make_shared< context_type >(
		readable_, std::move( options ) );

	context->fn = q::make_unique< fn_type >( std::forward< Fn >( fn ) );

	return context->consume( );
/* */
/* */
	typedef std::shared_ptr< detail::observable_readable< T > >
		readable_type;
	typedef typename q::objectify< typename std::decay< Fn >::type >::type
		fn_type;

	struct context_type
	{
		readable_type readable;
		std::unique_ptr< fn_type > fn;
		std::function< void( void ) > recursive_consumer;
		q::queue_ptr queue;
		std::size_t max_concurrency;
		std::atomic< std::size_t > concurrency;
		q::resolver< > resolver;
		q::rejecter< > rejecter;

		context_type( readable_type readable )
		: readable( readable )
		, concurrency( 0 )
		{ }
	};
	auto context = std::make_shared< context_type >( readable_ );

	// TODO: This will need to be rewritten to allow for void_t-wrapped
	// functions, if their arity is zero.
	context->fn = q::make_unique< fn_type >( std::forward< Fn >( fn ) );

	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( readable_->get_queue( ) ) ).value;
	context->queue = options.get< q::queue_ptr >( next_queue );
	context->max_concurrency = options.get< q::concurrency >( 1 );

	return q::make_promise_sync( context->queue,
		[ context ]( q::resolver< > resolve, q::rejecter< > reject )
	{
		context->resolver = resolve;
		context->rejecter = reject;

		context->recursive_consumer = [ context ]( )
		{
			context->readable->receive(
				[ context ]( void_safe_type&& t )
				{
					return ( *context->fn )( std::move( t ) )
					.then( [ context ]( )
					{
						context->recursive_consumer( );
					} );
				},
				context->queue
			)
			.fail( [ context ]( q::channel_closed_exception e )
			{
				context->resolver( );
			} )
			.fail( [ context ]( std::exception_ptr e )
			{
				context->rejecter( e );
			} );
		};

		context->recursive_consumer( );
	} )
	.use_queue( next_queue );
/* */
}

template< typename T >
template< typename Fn >
typename std::enable_if<
	q::arity_of_v< Fn > == 0
	and
	std::is_same< objectify_t< T >, void_t >::value,
	::q::promise< std::tuple< > >
>::type
observable< T >::
consume( Fn&& fn, base_options options )
{
	decayed_function_t< Fn > _fn{ std::forward< Fn >( fn ) };

	return consume( [ fn{ std::move( _fn ) } ]( void_t ) mutable
	{
		return fn( );
	}, options );
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_CONSUMERS_CONSUME_HPP
