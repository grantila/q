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

#ifndef LIBQ_RX_OBSERVABLE_OBSERVABLE_IMPL_CONSUME_HPP
#define LIBQ_RX_OBSERVABLE_OBSERVABLE_IMPL_CONSUME_HPP

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

	return q::make_promise_sync( context->queue,
		[ context ]( q::resolver< > resolve, q::rejecter< > reject )
	{
		context->resolver = resolve;
		context->rejecter = reject;

		struct worker
		{
			std::function< void( void ) > recursive_consumer;
			std::shared_ptr< q::detail::defer< > > defer;
		};

		auto this_worker = std::make_shared< worker >( );
		this_worker->defer = q::detail::defer< >::construct( context->queue );
		this_worker->recursive_consumer = [ context, this_worker ]( )
		{
			context->readable->receive(
				[ context, this_worker ]( T&& t )
				{
					( *context->fn )( std::move( t ) );

					this_worker->recursive_consumer( );
				},
				context->queue
			)
			.then( [ this_worker ]
			{
				this_worker->recursive_consumer = nullptr;
				this_worker->defer->set_value( );
			} )
			.fail( [ this_worker ]( std::exception_ptr )
			{
				this_worker->recursive_consumer = nullptr;
				this_worker->defer->set_value( );
			} );
		};

		this_worker->recursive_consumer( );

		this_worker->defer->get_promise( )
		.then( [ context ]( )
		{
			std::cout
				<< "GOT OK" << std::endl
				<< "  Closed: " << ( context->readable->is_closed( ) ? "yes" : "no" ) << std::endl
				<< "  Has error: " << ( context->readable->get_exception( ) ? "yes" : "no" ) << std::endl;
			auto exception = context->readable->get_exception( );
			if ( exception )
				std::rethrow_exception( exception );

			context->resolver( );
		} )
		.fail( [ context ]( q::channel_closed_exception e )
		{
			context->resolver( );
		} )
		.fail( [ context ]( std::exception_ptr e )
		{
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

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_OBSERVABLE_IMPL_CONSUME_HPP
