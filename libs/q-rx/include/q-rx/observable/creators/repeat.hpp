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

#ifndef LIBQ_RX_OBSERVABLE_CREATORS_REPEAT_HPP
#define LIBQ_RX_OBSERVABLE_CREATORS_REPEAT_HPP

namespace q { namespace rx {

template< typename T >
observable< T >
observable< T >::
repeat( combine_options options )
{
	return repeat( 0, options );
}

template< typename T >
observable< T >
observable< T >::
repeat( std::size_t limit, combine_options options )
{
	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( readable_->get_queue( ) ) ).value;
	auto backlog_size = options.get< backlog >( readable_->backlog( ) );

	channel_type channel_( next_queue, backlog_size.get( ) );
	writable_type writable = channel_.get_writable( );

	struct context
	{
		q::queue_ptr queue;

		std::function< void( ) > sender;
		// All saved values from the input observable
		std::vector< void_safe_type > values;
		// The maximum number of iterations
		std::size_t limit;
		// The loop count if `limit` is provided (i.e. non-zero)
		std::size_t iteration;
		// The next index we should sent from the values vector.
		std::size_t next_index;

		std::shared_ptr< detail::observable_readable< T > > readable;
		writable_type writable;

		std::shared_ptr< q::detail::defer< > > completion;

		// Back pressure notifications. This is from downstream up to
		// here. So we read notifications about when we can continue
		// sending.
		q::writable< > writable_back_pressure;
		q::readable< > readable_back_pressure;

		void cleanup( )
		{
			writable.unset_resume_notification( );
			readable.reset( );
			sender = std::function< void( ) >( );
			writable.close( );
			completion->set_value( );
		}

		void try_send( )
		{
			do
			{
				if ( !writable.send( values[ next_index++ ] ) )
					break;
				if ( next_index == values.size( ) )
				{
					next_index = 0;
					++iteration;

					if ( iteration > limit )
					{
						cleanup( );
						return;
					}
				}
			}
			while ( writable.should_send( ) );

			if ( writable.is_closed( ) )
				// Downstream closed when upstream is complete,
				// and all pumping is done here. We'll just
				// stop it and do no more.
				cleanup( );

			// We'll get back pressure notifications and
			// iterate this eventually
		}

		void start_repeat( const std::shared_ptr< context >& self )
		{
			++iteration; // Goes from 1 to 2

			// Stop using a back pressure channel, and listen in on
			// every callback instead.
			writable.set_resume_notification( [ self ]
			{
				self->try_send( );
			} );

			try_send( );
		}
	};

	auto ctx = std::make_shared< context >( );

	ctx->limit = limit;
	ctx->iteration = 1;
	ctx->next_index = 0;
	ctx->readable = readable_;
	ctx->writable = writable;
	ctx->queue = readable_->get_queue( );
	ctx->completion = q::detail::defer< >::construct( ctx->queue );

	q::channel< > notifications( ctx->queue, 1 );

	ctx->writable_back_pressure = notifications.get_writable( );
	ctx->readable_back_pressure = notifications.get_readable( );

	writable.set_resume_notification( [ ctx ]
	{
		ignore_result( ctx->writable_back_pressure.send( ) );
	} );

	// First, we consume all elements from the input, with the asynchronous
	// consume interface, respecting back pressure. We save each value, and
	// send it to the writable.
	//
	// A separate channel is used to signal back pressure notifications
	// from the writable, used to hold back the consumption.
	//
	// When we're done with all values, we'll go into another loop,
	// iterating these saved values over and over again to repeat n times.
	consume( [ ctx ]( void_safe_type t ) mutable
	-> q::promise< >
	{
		ctx->readable_back_pressure.clear( );

		if ( !ctx->writable.send( t ) )
			return q::reject< >(
				ctx->queue, ctx->writable.get_exception( ) );
		ctx->values.push_back( std::move( t ) );

		if ( ctx->writable.should_send( ) )
			return q::with( ctx->queue );

		else if ( ctx->writable.is_closed( ) )
			return q::reject< >(
				ctx->queue, ctx->writable.get_exception( ) );

		else
			return ctx->readable_back_pressure.receive( );
	} )
	.then( [ ctx, limit ]( ) mutable
	-> q::promise< >
	{
		// The input observable is now complete, with success. This is
		// when we take over and start repeating...
		if ( limit < 2 || ctx->values.empty( ) )
			ctx->cleanup( );
		else
			ctx->start_repeat( ctx );

		return ctx->completion->get_promise( );
	} )
	// Capturing ctx in this final fail handler will keep it alive exactly
	// as long as needed.
	.fail( [ ctx, writable ]( std::exception_ptr e ) mutable
	{
		// Any error on the input observable is immediately forwarded.
		// This is how all operators are expected to work.
		writable.close( e );
	} );

	return observable< T >( channel_ );
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_CREATORS_REPEAT_HPP
