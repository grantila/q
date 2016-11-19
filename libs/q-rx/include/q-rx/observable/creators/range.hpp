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

#ifndef LIBQ_RX_OBSERVABLE_CREATORS_RANGE_HPP
#define LIBQ_RX_OBSERVABLE_CREATORS_RANGE_HPP

namespace q { namespace rx {

template< typename T >
template< typename U >
typename std::enable_if<
	!std::is_void< U >::value
	and
	!std::is_same< void_t, typename std::decay< U >::type >::value,
	observable< T >
>::type
observable< T >::
range( U&& start, std::size_t count, create_options options )
{
	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( q::queue_ptr( ) ) ).value;
	auto queue = options.get< q::queue_ptr >( );
	auto backlog_size = options.get< backlog >( count );

	if ( !next_queue )
		next_queue = queue;

	channel_type channel_( queue, backlog_size );
	writable_type writable = channel_.get_writable( );

	if ( !options.has< backlog >( ) )
	{
		for ( std::size_t i = 0; i < count; ++i )
			if ( !writable.send( start++ ) )
				break;

		writable.close( );
	}
	else
	{
		auto cur = std::make_shared< std::size_t >( 0 );
		auto val = std::make_shared< T >( std::forward< U >( start ) );

		auto try_write = [ writable, cur, count, val ]( ) mutable
		{
			for ( ; *cur < count; ++*cur )
			{
				if ( writable.should_send( ) )
				{
					if ( !writable.send( ( *val )++ ) )
						break;
				}
				else
					break;
			}

			if ( *cur == count && !writable.is_closed( ) )
				writable.close( );
		};

		writable.set_resume_notification( try_write );

		queue->push( try_write );
	}

	return observable< T >( channel_ );
}

template< typename T >
template< typename U >
typename std::enable_if<
	std::is_void< U >::value,
	observable< T >
>::type
observable< T >::
range( std::size_t count, create_options options )
{
	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( q::queue_ptr( ) ) ).value;
	auto queue = options.get< q::queue_ptr >( );
	auto backlog_size = options.get< backlog >( count );

	if ( !next_queue )
		next_queue = queue;

	channel_type channel_( queue, backlog_size );
	writable_type writable = channel_.get_writable( );

	if ( !options.has< backlog >( ) )
	{
		for ( std::size_t i = 0; i < count; ++i )
			if ( !writable.send( ) )
				break;

		writable.close( );
	}
	else
	{
		auto cur = std::make_shared< std::size_t >( 0 );

		auto try_write = [ writable, cur, count ]( ) mutable
		{
			for ( ; *cur < count; ++*cur )
			{
				if ( writable.should_send( ) )
				{
					if ( !writable.send( ) )
						break;
				}
				else
					break;
			}

			if ( *cur == count && !writable.is_closed( ) )
				writable.close( );
		};

		writable.set_resume_notification( try_write );

		queue->push( try_write );
	}

	return observable< T >( channel_ );
}

template< typename T >
template< typename V, typename U >
typename std::enable_if<
	std::is_void< U >::value
	and
	std::is_same< void_t, typename std::decay< V >::type >::value,
	observable< T >
>::type
observable< T >::
range( V&& start, std::size_t count, create_options options )
{
	return range( count, options );
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_CREATORS_RANGE_HPP
