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

#ifndef LIBQ_RX_OBSERVABLE_CREATORS_START_HPP
#define LIBQ_RX_OBSERVABLE_CREATORS_START_HPP

namespace q { namespace rx {

namespace {

template< typename Writable, typename Fn >
bool write_by_fun( Writable& writable, Fn&& fn )
{
	return writable.send( fn( ) );
}

template< typename Fn >
bool write_by_fun( q::writable< > writable, Fn&& fn )
{
	fn( );
	return writable.send( );
}

template< typename T, typename Writable, typename Promise >
typename std::enable_if< !std::is_same< T, void >::value >::type
write_by_fun_async( Writable writable, Promise&& promise )
{
	promise
	.then( [ writable ]( T&& t ) mutable
	{
		if ( writable.send( std::move( t ) ) )
			writable.close( );
	} )
	.fail( [ writable ]( std::exception_ptr e ) mutable
	{
		writable.close( std::move( e ) );
	} );
}

template< typename T, typename Promise >
typename std::enable_if< std::is_same< T, void >::value >::type
write_by_fun_async( q::writable< > writable, Promise&& promise )
{
	promise
	.then( [ writable ]( ) mutable
	{
		if ( writable.send( ) )
			writable.close( );
	} )
	.fail( [ writable ]( std::exception_ptr e ) mutable
	{
		writable.close( std::move( e ) );
	} );
}

} // anonymous namespace

template< typename T >
template< typename Fn >
inline typename std::enable_if<
	!q::is_promise< q::result_of_t< Fn > >::value
	and
	std::is_same< q::result_of_t< Fn >, T >::value,
	observable< T >
>::type
observable< T >::
start( Fn&& fn, queue_options options )
{
	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( q::queue_ptr( ) ) ).value;
	auto queue = options.get< q::queue_ptr >( );

	if ( !next_queue )
		next_queue = queue;

	channel_type channel_( next_queue, 1 );
	writable_type writable = channel_.get_writable( );

	queue->push( [ writable, fn ]( ) mutable
	{
		try
		{
			write_by_fun( writable, std::move( fn ) );
			writable.close( );
		}
		catch ( ... )
		{
			writable.close( std::current_exception( ) );
		}
	} );

	return observable< T >( channel_ );
}

template< typename T >
template< typename Fn >
inline typename std::enable_if<
	q::is_promise< q::result_of_t< Fn > >::value
	and
	q::result_of_t< Fn >::argument_types
	::template is_convertible_to< q::arguments< T > >::value,
	observable< T >
>::type
observable< T >::
start( Fn&& fn, queue_options options )
{
	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( q::queue_ptr( ) ) ).value;
	auto queue = options.get< q::queue_ptr >( );

	if ( !next_queue )
		next_queue = queue;

	channel_type channel_( next_queue, 1 );
	writable_type writable = channel_.get_writable( );

	write_by_fun_async< T >(
		writable,
		q::with( queue )
		.then( std::move( fn ) )
	);

	return observable< T >( channel_ );
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_CREATORS_START_HPP
