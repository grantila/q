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

#ifndef LIBQ_RX_OBSERVABLE_CREATORS_TIMER_HPP
#define LIBQ_RX_OBSERVABLE_CREATORS_TIMER_HPP

namespace q { namespace rx {

template< typename T >
observable< T >
observable< T >::
timer( q::timer::duration_type duration, create_options options )
{
	return timer( duration, duration, std::move( options ) );
}

template< typename T >
observable< T >
observable< T >::
timer(
	q::timer::duration_type first_duration,
	q::timer::duration_type duration,
	create_options options
)
{
	return timer(
		duration,
		duration,
		q::default_initialize< T >( ),
		std::move( options )
	);
}

template< typename T >
template< typename U >
typename std::enable_if<
	q::is_argument_same_or_convertible_incl_void_t<
		q::arguments< typename std::decay< U >::type >,
		q::arguments< T >
	>::value
	and
	!std::is_same<
		typename std::decay< U >::type,
		q::timer::duration_type
	>::value,
	observable< T >
>::type
observable< T >::
timer( q::timer::duration_type duration, U&& value, create_options options )
{
	return timer(
		duration,
		duration,
		std::forward< U >( value ),
		std::move( options )
	);
}

template< typename T >
template< typename U >
typename std::enable_if<
	q::is_argument_same_or_convertible_incl_void_t<
		q::arguments< typename std::decay< U >::type >,
		q::arguments< T >
	>::value,
	observable< T >
>::type
observable< T >::
timer(
	q::timer::duration_type first_duration,
	q::timer::duration_type duration,
	U&& value,
	create_options options
)
{
	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( q::queue_ptr( ) ) ).value;
	auto queue = options.get< q::queue_ptr >( );
	auto backlog_size = options.get< backlog >( Q_RX_DEFAULT_BACKLOG );

	if ( !next_queue )
		next_queue = queue;

	typedef typename detail::suitable_promise_type< T >::type element_type;
	typedef q::channel< element_type > channel_type;
	typedef q::writable< element_type > writable_type;

	channel_type channel_( queue, backlog_size );
	writable_type writable = channel_.get_writable( );

	typedef std::size_t counter_t;

	struct context
	{
		std::shared_ptr< context > self_ptr;
		counter_t counter;
		q::timer::point_type base_time;
		q::timer::duration_type duration;
		q::queue_ptr queue;
		writable_type writable;
		typename std::decay< U >::type value;

		context( U&& _value )
		: value( std::move( _value ) )
		{ }

		context( const U& _value )
		: value( _value )
		{ }

		void setup( )
		{
			auto self = self_ptr;
			writable.set_resume_notification(
				[ self{ std::move( self ) } ]( ) mutable
				{
					self->try_send( );
				}
			);
		}

		void cleanup( )
		{
			writable.unset_resume_notification( );
			writable.close( );
			self_ptr.reset( );
		}

		void step( )
		{
			// Steps forward until numeric limit of counter_t (or
			// more precisely, until the highest bit is set, i.e.
			// after half the numeric limit), then bumps base_time
			// up, and resets counter.
			// Checking the highest bit with xor is fast, hence
			// this logic.

			++counter;
			if ( counter & bit_limits< counter_t >::highest_bit )
			{
				base_time += duration * counter;
				counter = 0;
			}
		}

		void try_send( )
		{
			// If we're spinning in the `while`, and then get a
			// close notification from another thread, we'll need
			// to ensure to keep a shared reference to this
			// context, or it'll be de-allocated while sending (or,
			// trying to send)
			std::shared_ptr< context > self = self_ptr;

			while ( writable.should_send( ) )
			{
				ignore_result( writable.send( q::delay(
					queue,
					base_time + duration * counter,
					value
				) ) );

				step( );
			}

			if ( writable.is_closed( ) )
				cleanup( );
		}
	};

	auto ctx = std::make_shared< context >(
		std::forward< U >( value )
	);

	ctx->self_ptr = ctx;
	ctx->counter = 0;
	ctx->base_time = q::timer::point_type::clock::now( ) + first_duration;
	ctx->duration = std::move( duration );
	ctx->queue = queue;
	ctx->writable = writable;

	ctx->setup( );
	queue->push( [ ctx ]( ) mutable
	{
		ctx->try_send( );
	} );

	return observable< T >( channel_ );
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_CREATORS_TIMER_HPP
