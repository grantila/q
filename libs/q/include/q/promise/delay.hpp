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

#ifndef LIBQ_PROMISE_DELAY_HPP
#define LIBQ_PROMISE_DELAY_HPP

namespace q {

template< typename... Args >
promise< typename std::decay< Args >::type... >
delay( const queue_ptr& queue, timer::point_type run_at, Args&&... args )
{
	typedef detail::defer< typename std::decay< Args >::type... >
		deferred_type;
	auto deferred = deferred_type::construct( queue );

	auto _args = std::make_tuple( std::forward< Args >( args )... );
	Q_MOVE_INTO_MOVABLE( _args );

	queue->push( [ deferred, Q_MOVABLE_MOVE( _args ) ]( ) mutable
	{
		deferred->set_value( Q_MOVABLE_CONSUME( _args ) );
	}, std::move( run_at ) );

	return deferred->get_promise( );
}

template< typename... Args >
promise< typename std::decay< Args >::type... >
delay( const queue_ptr& queue, timer::duration_type duration, Args&&... args )
{
	return delay(
		queue,
		timer::point_type::clock::now( ) + duration,
		std::forward< Args >( args )...
	);
}


} // namespace q

#endif // LIBQ_PROMISE_DELAY_HPP
