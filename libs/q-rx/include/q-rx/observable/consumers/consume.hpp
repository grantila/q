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

template< typename T >
template< typename Fn >
typename std::enable_if<
	Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( Fn, T )::value
	and
	std::is_void< ::q::result_of< Fn > >::value,
	::q::promise< std::tuple< > >
>::type
observable< T >::
consume( Fn&& fn, base_options options )
{
	typedef std::shared_ptr< detail::observable_readable< T > >
		readable_type;
	typedef typename std::decay< Fn >::type fn_type;

	struct context_type
	{
		readable_type readable;
		std::unique_ptr< fn_type > fn;
		q::queue_ptr queue;
		std::size_t max_concurrency;
		std::atomic< std::size_t > concurrency;
		std::shared_ptr< std::vector< q::promise< std::tuple< > > > >
			concurrent_readers;
		q::resolver< > resolver;
		q::rejecter< > rejecter;

		context_type( readable_type readable )
		: readable( readable )
		, concurrency( 0 )
		{ }
	};
	auto context = std::make_shared< context_type >( readable_ );

	context->fn = q::make_unique< fn_type >( std::forward< Fn >( fn ) );

	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( readable_->get_queue( ) ) ).value;
	context->queue = options.get< q::queue_ptr >( next_queue );
	auto concurrency = options.get< q::concurrency >( 1 );
	// Ensure we don't use "infinite" concurrency, as this would loop
	context->max_concurrency = concurrency == q::concurrency( )
		? q::soft_cores( )
		: static_cast< std::size_t >( concurrency );
	if ( context->max_concurrency == 0 )
		context->max_concurrency = 1;

	return q::make_promise_sync( context->queue,
		[ context ]( q::resolver< > resolve, q::rejecter< > reject )
	{
		context->resolver = resolve;
		context->rejecter = reject;

		context->concurrent_readers = std::make_shared<
			std::vector< q::promise< std::tuple< > > >
		>( );

		while ( context->concurrency < context->max_concurrency )
		{
			context->concurrency += 1;

			context->concurrent_readers->push_back(
				q::make_promise_sync(
					context->queue,
					[ context ]
			( q::resolver< > resolve, q::rejecter< > reject )
			{
				struct worker
				{
					std::function< void( void ) >
						recursive_consumer;
					q::resolver< > resolve;
					q::rejecter< > reject;
				};

				auto this_worker =
					std::make_shared< worker >( );

				this_worker->resolve = resolve;
				this_worker->reject = reject;

				this_worker->recursive_consumer =
					[ context, this_worker ]( )
				{
					auto fn = *context->fn;
					context->readable->receive(
						fn,
						context->queue
					)
					.then( [ this_worker ]( )
					{
						// No error, there might be
						// more data to read
						this_worker->
							recursive_consumer( );
					} )
					.fail( [ this_worker ](
						const q::channel_closed_exception&
					)
					{
						// Channel closed successfully
						this_worker->
							recursive_consumer =
							nullptr;
						this_worker->resolve( );
					} )
					.fail( [ this_worker ](
						std::exception_ptr e
					)
					{
						// Channel closed with error
						this_worker->
							recursive_consumer =
							nullptr;
						this_worker->reject( e );
					} );
				};

				this_worker->recursive_consumer( );
			} ) );
		}

		q::all( *context->concurrent_readers, context->queue )
		.strip( )
		.then( [ context ]( )
		{
			auto exception = context->readable->get_exception( );
			if ( exception )
				std::rethrow_exception( exception );

			// This should likely never happen
			context->resolver( );
		} )
		.fail( [ context ]( q::channel_closed_exception e )
		{
			context->resolver( );
		} )
		.fail( [ context ](
			const q::combined_promise_exception< void >& e
		)
		{
			if ( e.exceptions( ).empty( ) )
				// This should never happen
				context->rejecter( e );
			else
				context->rejecter( e.exceptions( )[ 0 ] );
		} )
		.fail( [ context ]( std::exception_ptr e )
		{
			// This should never happen
			context->rejecter( e );
		} );
	} )
	.use_queue( next_queue );
}

template< typename T >
template< typename Fn >
typename std::enable_if<
	Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( Fn, T )::value
	and
	::q::is_promise<
		typename std::decay< ::q::result_of< Fn > >::type
	>::value
	and
	::q::result_of< Fn >::argument_types::size::value == 0,
	::q::promise< std::tuple< > >
>::type
observable< T >::
consume( Fn&& fn, base_options options )
{
	typedef std::shared_ptr< detail::observable_readable< T > >
		readable_type;
	typedef typename std::decay< Fn >::type fn_type;

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
				[ context ]( T&& t )
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
}

template< typename T >
template< typename Fn >
typename std::enable_if<
	Q_ARGUMENTS_ARE( Fn, void_t )::value
	and
	std::is_same< T, void >::value,
	::q::promise< std::tuple< > >
>::type
observable< T >::
consume( Fn&& fn, base_options options )
{
	Q_MAKE_MOVABLE( fn );

	return consume( [ Q_MOVABLE_MOVE( fn ) ]( ) mutable
	{
		Q_MOVABLE_GET( fn )( void_t( ) );
	}, options );
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_CONSUMERS_CONSUME_HPP
