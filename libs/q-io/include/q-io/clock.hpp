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

#ifndef LIBQIO_CLOCK_HPP
#define LIBQIO_CLOCK_HPP

#include <chrono>

#include <sys/time.h>

namespace q { namespace io {

/**
 * A clock type fulfilling the TrivialClock concept. The native type behind
 * this clock type are the same as the IO backend's internal clock type.
 *
 * NOTE; This clock may be slow to fetch the RTC (using now( ) i.e.).
 */
class clock
{
public:
	typedef std::chrono::microseconds        duration;
	typedef duration::rep                    rep;
	typedef duration::period                 period;
	typedef std::chrono::time_point< clock > time_point;

	static const bool is_steady = false;

	static time_point now( );
	static time_point from_timeval( const timeval& tv );
	static timeval    to_timeval( const time_point& tp );
	static timeval    to_timeval( const duration& dur );
};

} } // namespace io, namespace q

#endif // LIBQIO_CLOCK_HPP
