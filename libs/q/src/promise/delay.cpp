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

#include <q/promise.hpp>

namespace q {

promise< std::tuple< > >
delay( const queue_ptr& queue, timer::point_type run_at )
{
	auto deferred = ::q::detail::defer< >::construct( queue );

	queue->push( [ deferred ]( ) mutable
	{
		deferred->set_value( );
	}, std::move( run_at ) );

	return deferred->get_promise( );
}

promise< std::tuple< > >
delay( const queue_ptr& queue, timer::duration_type duration )
{
	return delay( queue, timer::point_type::clock::now( ) + duration );
}

} // namespace q
