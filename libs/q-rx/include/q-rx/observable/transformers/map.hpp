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

#ifndef LIBQ_RX_OBSERVABLE_TRANSFORMERS_MAP_HPP
#define LIBQ_RX_OBSERVABLE_TRANSFORMERS_MAP_HPP

namespace q { namespace rx {

/**
 * ( In ) -> Out
 */
template< typename T >
template< typename Fn >
typename std::enable_if<
	q::arguments_of_are_convertible_from_incl_void< Fn, T >
	and
	!q::is_promise< Q_RESULT_OF( Fn ) >::value,
	observable< Q_RESULT_OF( Fn ) >
>::type
observable< T >::
map( Fn&& fn, base_options options )
{
	typedef Q_RESULT_OF( Fn ) Out;
	typedef typename std::conditional<
		std::is_void< Out >::value,
		std::tuple< >,
		std::tuple< Out >
	>::type out_tuple_type;
	typedef q::promise< out_tuple_type > out_promise_type;

	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( readable_->get_queue( ) ) ).value;
	auto queue = options.get< q::queue_ptr >( next_queue );
	auto concurrency = options.get< q::concurrency >( 1 );

	::q::channel< out_promise_type > ch( next_queue, concurrency );

	// TODO: Implement concurrency
	// TODO: Implement back pressure handling

	auto writable = ch.get_writable( );

	consume( [ queue, writable, fn ]( void_safe_type t ) mutable
	{
		auto deferred = ::q::detail::defer< out_tuple_type >
			::construct( queue );

		writable.send( deferred->get_promise( ) );

		queue->push( [ deferred, fn, t{ std::move( t ) } ]( )
		{
			deferred->set_by_fun( fn, std::move( t ) );
		} );
	} )
	.then( [ writable ]( ) mutable
	{
		writable.close( );
	} )
	.fail( [ writable ]( std::exception_ptr e ) mutable
	{
		writable.close( e );
	} );

	return observable< Out >( ch.get_readable( ) );
}

/**
 * ( In ) -> promise< Out >
 */
template< typename T >
template< typename Fn >
typename std::enable_if<
	q::arguments_of_are_convertible_from_incl_void< Fn, T >
	and
	q::is_promise< Q_RESULT_OF( Fn ) >::value,
	typename detail::tuple_to_observable<
		typename ::q::result_of< Fn >::tuple_type
	>::type
>::type
observable< T >::
map( Fn&& fn, base_options options )
{
	// TODO: Implement
	return *this;
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_TRANSFORMERS_MAP_HPP
