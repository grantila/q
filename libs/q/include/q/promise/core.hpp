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

#ifndef LIBQ_PROMISE_CORE_HPP
#define LIBQ_PROMISE_CORE_HPP

#include <q/exception.hpp>

namespace q {

template< typename T >
class promise;

template< typename T >
class shared_promise;

namespace detail { template< bool, typename > class generic_promise; }

template< class T >
struct is_promise
: std::false_type
{ };

template< bool B, typename T >
struct is_promise< detail::generic_promise< B, T > >
: std::true_type
{ };

template< class T >
struct is_promise< promise< T > >
: std::true_type
{ };

template< class T >
struct is_promise< shared_promise< T > >
: std::true_type
{ };

template< class... T >
struct are_promises
: fold<
	q::arguments< T... >,
	generic_operator<
		is_promise, logic_and
	>::template fold_type,
	std::true_type
>
{ };

// TODO: Make this exception actually chain the original exception that was thrown..
class broken_promise_exception
: exception
{
public:
	broken_promise_exception( ) = delete;

	// TODO: Something...
	broken_promise_exception( std::exception_ptr&& e )
	{ }
};

class generic_combined_promise_exception
: exception
{
public:
	generic_combined_promise_exception( ) = default;

	const std::vector< std::exception_ptr >& exceptions( ) const
	{
		return exceptions_;
	}

protected:
	void add_exception( const std::exception_ptr& e )
	{
		exceptions_.push_back( e );
	}

private:
	std::vector< std::exception_ptr > exceptions_;
};

template< typename T >
class combined_promise_exception
: generic_combined_promise_exception
{
public:
	typedef std::vector< expect< T > > exception_type;

	combined_promise_exception( ) = delete;
	combined_promise_exception( std::vector< expect< T > >&& data )
	: data_( new exception_type( std::move( data ) ) )
	{
		for ( auto& element : *data_ )
			if ( element.has_exception( ) )
				add_exception( element.exception( ) );
	}

	const std::vector< expect< T > >& data( )
	{
		return *data_;
	}

	std::vector< expect< T > > consume( )
	{
		return std::move( *data_ );
	}

private:
	// The data needs to be shared_ptr'd as C++ internally may copy the data
	// when dealing with std::exception_ptr's.
	std::shared_ptr< exception_type > data_;
};

}

#endif // LIBQ_PROMISE_CORE_HPP
