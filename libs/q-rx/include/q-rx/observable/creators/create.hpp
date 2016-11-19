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

#ifndef LIBQ_RX_OBSERVABLE_CREATORS_CREATE_HPP
#define LIBQ_RX_OBSERVABLE_CREATORS_CREATE_HPP

namespace q { namespace rx {

template< typename T >
inline observable< T > observable< T >::
create( std::function< void( observer ) > fn, create_options options )
{
	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( q::queue_ptr( ) ) ).value;
	auto queue = options.get< q::queue_ptr >( );
	auto backlog_size = options.get< backlog >( 1 );

	if ( !next_queue )
		next_queue = queue;

	if ( !next_queue )
		// TODO: Make real exception
		Q_THROW( q::exception( ), "Missing queue" );

	q::channel< T > channel_( next_queue, backlog_size );

	observer obs( channel_.get_writable( ) );

	queue->push( [ fn, obs ]( ) mutable
	{
		try
		{
			fn( obs );
		}
		catch ( ... )
		{
			obs.on_error( std::current_exception( ) );
		}
	} );

	return observable< T >( channel_ );
}

template< typename T >
inline observable< T >::observer::observer( q::writable< T > writable )
: writable_( writable )
{ }

template< typename T >
inline bool observable< T >::observer::on_next( T&& t )
{
	return writable_.send( std::move( t ) );
}

template< typename T >
inline bool observable< T >::observer::on_next( const T& t )
{
	return writable_.send( t );
}

template< typename T >
inline void observable< T >::observer::on_completed( )
{
	writable_.close( );
}

template< typename T >
template< typename Error >
inline void observable< T >::observer::on_error( Error&& error )
{
	writable_.close( std::forward< Error >( error ) );
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_CREATORS_CREATE_HPP
