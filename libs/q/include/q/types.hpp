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

#ifndef LIBQ_TYPES_HPP
#define LIBQ_TYPES_HPP

#include <q/pp.hpp>

#include <memory>
#include <functional>
#include <string>
#include <sstream>

// TODO: Remove this in favor of proper logging
#include <iostream>

namespace q {

#define Q_HERE \
	::q::macro_location( { LIBQ_FILE, LIBQ_LINE, LIBQ_FUNCTION } )

namespace detail {
	template< typename T >
	T type_identity( T&& );

	template< typename T >
	struct ensure_pointer
	{
		typedef T type;
	};

	template< typename T, std::size_t I >
	struct ensure_pointer< T( & )[ I ] >
	{
		typedef T* type;
	};
} // namespace detail

struct macro_line
{
	typedef decltype( detail::type_identity( LIBQ_LINE ) ) type;
};

struct macro_file
{
	typedef decltype( detail::type_identity( LIBQ_FILE ) ) raw_type;
	typedef detail::ensure_pointer< raw_type >::type type;
};

namespace detail {	

//static const auto function_macro_type = [ ]( ) { return LIBQ_FUNCTION; };

} // namespace detail

struct macro_function
{
	typedef macro_file::type type;
	//typedef decltype( detail::function_macro_type( ) ) raw_type;
	//typedef detail::ensure_pointer< raw_type >::type type;
};

class macro_location
{
public:
	macro_location( )
	: valid_( false )
	, file_( )
	, line_( )
	, function_( )
	{ }

	macro_location( macro_file::type     file,
	                macro_line::type     line,
	                macro_function::type function )
	: valid_( true )
	, file_( file )
	, line_( line )
	, function_( function )
	{ }

	bool valid( ) const
	{
		return valid_;
	}
	macro_file::type file( ) const
	{
		return file_;
	}
	macro_line::type line( ) const
	{
		return line_;
	}
	macro_function::type function( ) const
	{
		return function_;
	}

	std::string string( ) const
	{
		if ( valid_ )
		{
			std::stringstream ss;
			ss << file_ << ":" << line_ << ": " << function_;
			return ss.str( );
		}
		else
			return "(location unknown)";
	}

private:
	bool                 valid_;
	macro_file::type     file_;
	macro_line::type     line_;
	macro_function::type function_;
};

class threadpool;

class queue;
typedef std::shared_ptr< queue > queue_ptr;

class scheduler;

typedef int priority_t;

typedef std::function< void( void ) noexcept > task;

} // namespace q

#endif // LIBQ_TYPES_HPP
