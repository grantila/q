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

namespace { static std::atomic< int > _id( 0 ); }

template< typename T >
template< typename Fn, typename Queue >
typename std::enable_if<
	Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( Fn, T )::value
	and
	std::is_void< ::q::result_of< Fn > >::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	::q::promise< std::tuple< > >
>::type
observable< T >::
consume( Fn&& fn, Queue&& queue )
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
		bool set_default;
		q::resolver< > resolver;
		q::rejecter< > rejecter;

		context_type( readable_type readable )
		: readable( readable )
		, set_default( false )
		{ }
	};
	auto context = std::make_shared< context_type >( readable_ );

	context->fn = q::make_unique< fn_type >( std::forward< Fn >( fn ) );

	if ( is_set_default< Queue >::value )
	{
		context->queue = ensure( set_default_forward( queue ) );
		context->set_default = true;
	}
	else
	{
		context->queue = ensure( std::forward< Queue >( queue ) );
	}

	int id = ++_id;

	return q::make_promise_sync( context->queue,
		[ context, id ]( q::resolver< > resolve, q::rejecter< > reject )
	{
		context->resolver = resolve;
		context->rejecter = reject;

		context->recursive_consumer = [ context, id ]( )
		{
			std::cout << "--------- trying consume on " << id << std::endl;
			auto promise = context->readable->receive(
				[ context, id ]( T&& t )
				{
					std::cout << "--------- consume on " << id << " got " << t << std::endl;
					( *context->fn )( std::move( t ) );

					context->recursive_consumer( );
				},
				context->queue
			);

			if ( context->set_default )
				promise = promise.use_queue( context->queue );

			promise
			.fail( [ context, id ]( q::channel_closed_exception e )
			{
				std::cout << "--------- consume on " << id << " closed/resolve" << std::endl;
				context->resolver( );
			} )
			.fail( [ context, id ]( std::exception_ptr e )
			{
				std::cout << "--------- consume on " << id << " rejected" << std::endl;
				context->rejecter( e );
			} );
		};

		context->recursive_consumer( );
	} );
}

template< typename T >
template< typename Fn, typename Queue >
typename std::enable_if<
/*	Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( Fn, T )::value
	and*/
		::q::is_promise< typename std::decay< ::q::result_of< Fn > >::type >::value
	and
	::q::result_of< Fn >::argument_types::size::value == 0
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	::q::promise< std::tuple< > >
>::type
observable< T >::
consume( Fn&& fn, Queue&& queue )
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
		bool set_default;
		q::resolver< > resolver;
		q::rejecter< > rejecter;

		context_type( readable_type readable )
		: readable( readable )
		, set_default( false )
		{ }
	};
	auto context = std::make_shared< context_type >( readable_ );

	context->fn = q::make_unique< fn_type >( std::forward< Fn >( fn ) );

	if ( is_set_default< Queue >::value )
	{
		context->queue = ensure( set_default_forward( queue ) );
		context->set_default = true;
	}
	else
	{
		context->queue = ensure( std::forward< Queue >( queue ) );
	}

	return q::make_promise_sync( context->queue,
		[ context ]( q::resolver< > resolve, q::rejecter< > reject )
	{
		context->resolver = resolve;
		context->rejecter = reject;

		context->recursive_consumer = [ context ]( )
		{
			auto promise = context->readable->receive(
				[ context ]( T&& t )
				{
					return ( *context->fn )( std::move( t ) )
					.then( [ context ]( )
					{
						context->recursive_consumer( );
					} );
				},
				context->queue
			);

			if ( context->set_default )
				promise = promise.use_queue( context->queue );

			promise
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
	} );
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_OBSERVABLE_IMPL_CONSUME_HPP
