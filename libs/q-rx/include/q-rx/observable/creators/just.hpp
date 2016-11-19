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

#ifndef LIBQ_RX_OBSERVABLE_CREATORS_JUST_HPP
#define LIBQ_RX_OBSERVABLE_CREATORS_JUST_HPP

namespace q { namespace rx {

namespace {

template< typename T >
static bool append_writable( q::writable< T >& w )
{
	return true;
}

template< typename T, typename First, typename... Rest >
static bool append_writable( q::writable< T >& w, First&& first, Rest&&... rest )
{
	if ( !w.send( std::forward< First >( first ) ) )
		return false;
	return append_writable( w, std::forward< Rest >( rest )... );
}

} // anonymous namespace

template< typename T >
template< typename... U >
typename std::enable_if<
	q::are_all_same<
		T,
		typename std::decay< U >::type...
	>::value,
	observable< T >
>::type
observable< T >::
just( const queue_ptr& queue, U&&... values )
{
	q::channel< T > channel_( queue, sizeof...( U ) );
	q::writable< T > writable = channel_.get_writable( );

	if ( append_writable( writable, std::forward< U >( values )... ) )
		writable.close( );

	return observable< T >( channel_ );
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_CREATORS_JUST_HPP
