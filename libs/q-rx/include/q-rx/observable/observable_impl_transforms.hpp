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

#ifndef LIBQ_RX_OBSERVABLE_OBSERVABLE_IMPL_TRANSFORMS_HPP
#define LIBQ_RX_OBSERVABLE_OBSERVABLE_IMPL_TRANSFORMS_HPP

namespace q { namespace rx {

/**
 * ( In ) -> Out
 */
template< typename T >
template< typename Fn >
typename std::enable_if<
	!q::is_promise< Q_RESULT_OF( Fn ) >::value,
	observable< Q_RESULT_OF( Fn ) >
>::type
observable< T >::
map( Fn&& fn )
{
	typedef Q_RESULT_OF( Fn ) Out;
	typedef q::arguments< Out > out_arguments_type;
	typedef std::tuple< Out > out_tuple_type;
	typedef q::promise< out_tuple_type > out_promise_type;

	auto queue = readable_->get_queue( );
	auto next_queue = queue;

	::q::channel< out_promise_type > ch( next_queue, 1 );

	auto writable = ch.get_writable( );

	consume( [ queue, writable, fn ]( T t ) mutable
	{
		auto deferred = ::q::detail::defer< out_tuple_type >
			::construct( queue );

		writable.send( deferred->get_promise( ) );

		queue->push( [ deferred, fn, t ]( )
		{
			try
			{
				deferred->set_value( fn( std::move( t ) ) );
			}
			catch( ... )
			{
				deferred->set_exception(
					std::current_exception( ) );
			}
		} );
	} )
	.then( [ writable ]( ) mutable
	{
		writable.close( );
	} )
	.fail( [ writable, queue ]( std::exception_ptr e ) mutable
	{
		writable.send( ::q::reject< out_arguments_type >( queue, std::move( e ) ) );
	} );

	return observable< Out >( ch.get_readable( ) );
}

/**
 * ( In ) -> promise< Out >
 */
template< typename T >
template< typename Fn >
typename std::enable_if<
	q::is_promise< Q_RESULT_OF( Fn ) >::value,
	typename detail::tuple_to_observable<
		typename ::q::result_of< Fn >::tuple_type
	>::type
>::type
observable< T >::
map( Fn&& fn )
{
	return *this;
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_OBSERVABLE_IMPL_TRANSFORMS_HPP
