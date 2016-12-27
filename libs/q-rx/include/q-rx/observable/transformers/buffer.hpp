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

#ifndef LIBQ_RX_OBSERVABLE_TRANSFORMERS_BUFFER_HPP
#define LIBQ_RX_OBSERVABLE_TRANSFORMERS_BUFFER_HPP

namespace q { namespace rx {

/**
 * count -> ( count, skip )
 */
template< typename T >
observable< std::vector< typename observable< T >::void_safe_type > >
observable< T >::
buffer( std::size_t count, combine_options options )
{
	return buffer( count, count, options );
}

/**
 * count, stride
 */
template< typename T >
observable< std::vector< typename observable< T >::void_safe_type > >
observable< T >::
buffer( std::size_t count, std::size_t stride, combine_options options )
{
	if ( count == 0 )
		Q_THROW( q::length_error( "Cannot buffer with count zero" ) );

	if ( count > stride )
		Q_THROW( q::length_error(
			"Cannot buffer with count greater than stride" ) );

	auto queue = readable_->get_queue( );
	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( queue ) ).value;
	auto backlog_size = options.get< backlog >( Q_RX_DEFAULT_BACKLOG );

	typedef q::channel< std::vector< void_safe_type > > channel_type;

	channel_type channel_( next_queue, backlog_size );
	auto writable = channel_.get_writable( );

	struct context
	{
		q::readable< > back_pressure_readable;
		queue_ptr queue;
		q::writable< std::vector< void_safe_type > > writable;
		std::vector< void_safe_type > buf;
		std::size_t count;
		std::size_t stride;
		std::size_t index;

		context(
			queue_ptr queue,
			q::writable< std::vector< void_safe_type > > writable
		)
		: queue( queue )
		, writable( writable )
		, index( 0 )
		{
			q::channel< > bp( queue, 1 );
			back_pressure_readable = bp.get_readable( );
			auto back_pressure_writable = bp.get_writable( );

			writable.set_resume_notification(
				[ back_pressure_writable ]( ) mutable
				{
					ignore_result( back_pressure_writable.send( ) );
				}
			);
		}

		~context( )
		{
			cleanup( );
		}

		void cleanup( )
		{
			if ( !buf.empty( ) )
			{
				// There is an unsent last chunk of data.
				// We ignore the errors, since upstream is
				// already closed, and have no chance to signal
				// errors.
				ignore_result(
					writable.send( std::move( buf ) ) );
				reset( false );
			}

			writable.unset_resume_notification( );
			writable.close( );
		}

		void reset( bool reserve = true )
		{
			decltype( buf ) new_buf;
			std::swap( new_buf, buf );

			buf.clear( );
			if ( reserve )
				buf.reserve( count );
			index = 0;
		}

		bool step( void_safe_type&& t )
		{
			if ( index < count )
				buf.push_back( std::move( t ) );

			++index;

			if ( index != stride )
				// Keep reading (and maybe buffering) before we
				// reach the stride
				return true;

			if ( writable.is_closed( ) )
				std::rethrow_exception(
					writable.get_exception( ) );

			if ( writable.send( std::move( buf ) ) )
			{
				reset( );
			}
			else
			{
				// Reset to not leave a moved vector dangling
				reset( );

				// We'll probably a channel_closed_exception,
				// but what we really want is to propagate the
				// real inner exception upstream.
				std::rethrow_exception(
					writable.get_exception( ) );
			}

			if ( !writable.should_send( ) )
			{
				// We shouldn't send any more right now.
				// We'll await downstream back pressure
				// notification for upstream resuming.
				back_pressure_readable.clear( );
				writable.trigger_resume_notification( );

				return false;
			}

			return true;
		}

		q::promise< > on_data( void_safe_type&& t )
		{
			if ( step( std::move( t ) ) )
				// We can keep getting more data
				return q::with( queue );
			else
				// We need to await downstream back pressure
				return back_pressure_readable.receive( );
		}
	};

	auto ctx = std::make_shared< context >( queue, writable );

	ctx->count = count;
	ctx->stride = stride;

	consume( [ ctx ]( void_safe_type&& t )
	{
		return ctx->on_data( std::move( t ) );
	} )
	.fail( [ writable ]( std::exception_ptr e ) mutable
	{
		// Any error on the input observable is immediately forwarded.
		// This is how all operators are expected to work.
		writable.close( e );
	} )
	.finally( [ ctx ]( )
	{
		ctx->cleanup( );
	} );

	return observable< std::vector< void_safe_type > >( channel_ );
}

/**
 * closing_observable
 */
template< typename T >
observable< std::vector< typename observable< T >::void_safe_type > >
observable< T >::
buffer( observable< void > closing_observable, combine_options options )
{
	auto queue = readable_->get_queue( );
	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( queue ) ).value;
	auto backlog_size = options.get< backlog >( Q_RX_DEFAULT_BACKLOG );

	typedef q::channel< std::vector< void_safe_type > > channel_type;

	channel_type channel_( next_queue, backlog_size );
	auto writable = channel_.get_writable( );

	struct context
	{
		q::mutex mut;
		q::readable< > back_pressure_readable;
		queue_ptr queue;
		q::writable< std::vector< void_safe_type > > writable;
		std::vector< void_safe_type > buf;

		context(
			queue_ptr queue,
			q::writable< std::vector< void_safe_type > > writable
		)
		: mut( Q_HERE, "observable::buffer(closing_observable)" )
		, queue( queue )
		, writable( writable )
		{
			q::channel< > bp( queue, 1 );
			back_pressure_readable = bp.get_readable( );
			auto back_pressure_writable = bp.get_writable( );

			auto notification = [ back_pressure_writable ]( )
			mutable
			{
				ignore_result(
					back_pressure_writable.send( ) );
			};

			writable.set_resume_notification(
				std::move( notification ) );
		}

		~context( )
		{
			cleanup( );
		}

		void cleanup( )
		{
			if ( !buf.empty( ) )
			{
				// There is an unsent last chunk of data.
				try
				{
					writable.send( std::move( buf ) );
				}
				// We ignore the errors, since upstream is
				// already closed, and have no chance to signal
				// errors.
				catch ( std::exception_ptr e ) { }
			}

			writable.unset_resume_notification( );
			writable.close( );
		}

		q::promise< > push( )
		{
			decltype( buf ) swapped_buf;

			Q_AUTO_UNIQUE_LOCK( mut );

			std::swap( swapped_buf, buf );

			if ( writable.is_closed( ) )
				std::rethrow_exception(
					writable.get_exception( ) );

			try
			{
				writable.send( std::move( swapped_buf ) );
			}
			catch ( std::exception_ptr err )
			{
				// We'll probably a channel_closed_exception,
				// but what we really want is to propagate the
				// real inner exception upstream.
				std::rethrow_exception(
					writable.get_exception( ) );
			}

			if ( !writable.should_send( ) )
			{
				// We shouldn't send any more right now.
				// We'll await downstream back pressure
				// notification for upstream resuming.
				back_pressure_readable.clear( );
				writable.trigger_resume_notification( );

				// We need to await downstream back pressure
				return back_pressure_readable.receive( );
			}

			return q::with( queue );
		}

		void on_data( void_safe_type&& t )
		{
			Q_AUTO_UNIQUE_LOCK( mut );

			buf.push_back( std::move( t ) );
		}
	};

	auto ctx = std::make_shared< context >( queue, writable );

	// Consume from the upstream observable

	consume( [ ctx ]( void_safe_type&& t )
	{
		return ctx->on_data( std::move( t ) );
	} )
	.fail( [ writable ]( std::exception_ptr e ) mutable
	{
		// Any error on the input observable is immediately forwarded.
		// This is how all operators are expected to work.
		writable.close( e );
	} )
	.finally( [ ctx ]( )
	{
		ctx->cleanup( );
	} );

	// Consume from the closing observable

	closing_observable.consume( [ ctx ]( )
	{
		return ctx->push( );
	} )
	.fail( [ writable ]( std::exception_ptr e ) mutable
	{
		// Any error on the closing observable is immediately forwarded.
		// This is the only way to ensure this observable isn't stuck
		// forever, if the upstream observable is never ending.
		writable.close( e );
	} )
	.finally( [ ctx ]( )
	{
		ctx->cleanup( );
	} );

	return observable< std::vector< void_safe_type > >( channel_ );
}

/**
 * duration -> ( duration, shift )
 */
template< typename T >
observable< std::vector< typename observable< T >::void_safe_type > >
observable< T >::
buffer( q::timer::duration_type duration, combine_options options )
{
	return buffer(
		std::move( duration ),
		q::timer::duration_type::zero( ),
		std::move( options )
	);
}

/**
 * duration, shift
 */
template< typename T >
observable< std::vector< typename observable< T >::void_safe_type > >
observable< T >::
buffer(
	q::timer::duration_type duration,
	q::timer::duration_type shift,
	combine_options options
)
{
	; // TODO
}

/**
 * duration, count
 */
template< typename T >
observable< std::vector< typename observable< T >::void_safe_type > >
observable< T >::
buffer(
	q::timer::duration_type duration,
	std::size_t count,
	combine_options options
)
{
	; // TODO
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_TRANSFORMERS_BUFFER_HPP
