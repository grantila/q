/*
 * Copyright 2013 Gustaf Räntilä
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

#ifndef LIBQ_TIMER_HPP
#define LIBQ_TIMER_HPP

#include <q/promise/async_task.hpp>

#include <chrono>

namespace q {

class timer
{
public:
	typedef std::chrono::high_resolution_clock::time_point point_type;
	typedef std::chrono::high_resolution_clock::duration duration_type;

	timer( )
	: before_( std::chrono::high_resolution_clock::now( ) )
	{ }

	duration_type diff( ) const
	{
		point_type now = std::chrono::high_resolution_clock::now( );

		auto diff = now - before_;

		// std::chrono::nanoseconds

		// before_ += now - before_;

		return diff;
	}

private:
	point_type before_;
};

class scoped_timer
{
public:
	typedef std::function< void( timer::duration_type&& ) > function_type;

	scoped_timer( function_type fn )
	: fn_( std::move( fn ) )
	, timer_( )
	{ }

	~scoped_timer( )
	{
		fn_( timer_.diff( ) );
	}

private:
	function_type fn_;
	timer timer_;
};

/**
 * The `timer_dispatcher` base class is strictly virtual in q, meaning that it
 * needs to be implemented by other libraries or user code.
 *
 * There is an implementation in q-io, called `q::io::dispatcher`.
 */
class timer_dispatcher
{
public:
	virtual async_task delay( timer::duration_type dur ) = 0;
};

} // namespace q

#endif // LIBQ_TIMER_HPP
