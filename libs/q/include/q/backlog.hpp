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

#ifndef LIBQ_BACKLOG_HPP
#define LIBQ_BACKLOG_HPP

#include <q/types.hpp>
#include <q/type_traits.hpp>

namespace q {

class backlog
{
public:
	enum other
	{
		infinity
	};

	backlog( other value )
	: value_( std::numeric_limits< std::size_t >::max( ) )
	{ }

	backlog( std::size_t value )
	: value_( value )
	{ }

	// Will default to 1
	backlog( )
	: value_( 0 )
	{ }

	operator std::size_t( ) const
	{
		if ( !value_ )
			return 1;
		return value_;
	}

	std::size_t get( std::size_t fallback = 1 ) const
	{
		if ( !value_ )
			return fallback;
		return value_;
	}

private:
	std::size_t value_;
};

} // namespace q

#endif // LIBQ_BACKLOG_HPP
