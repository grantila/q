/*
 * Copyright 2014 Gustaf Räntilä
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

#include <q-io/clock.hpp>

namespace q { namespace io {

clock::time_point clock::now( )
{
	timeval tv;

	// TODO: Handle return value
	gettimeofday( &tv, nullptr );

	return from_timeval( tv );
}

clock::time_point clock::from_timeval( const timeval& tv )
{
	rep ticks = static_cast< rep >( tv.tv_sec ) * 1000 * 1000 +
	            static_cast< rep >( tv.tv_usec );

	return time_point( duration( ticks ) );
}

timeval clock::to_timeval( const time_point& tp )
{
	return to_timeval( tp.time_since_epoch( ) );
}

timeval clock::to_timeval( const duration& dur )
{
	timeval tv;

	auto usec = std::chrono::duration_cast< std::chrono::microseconds >(
		dur ).count( );

	time_t seconds = usec / ( 1000 * 1000 );

	tv.tv_sec  = seconds;
	tv.tv_usec = usec - seconds * ( 1000 * 1000 );

	return tv;
}

} } // namespace io, namespace q
