/*
 * Copyright 2015 Gustaf Räntilä
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

#ifndef LIBQ_OPTIONS_HPP
#define LIBQ_OPTIONS_HPP

#include <q/type_traits.hpp>

namespace q {

namespace detail {

template< typename T >
struct single_option
{
	typedef single_option< T > this_type;

	single_option( )
	: set_( false )
	{ }

	template<
		typename Tuple,
		typename std::enable_if<
			tuple_arguments< Tuple >
				::template index_of< T >::value != -1
		>::type* = nullptr
	>
	single_option( Tuple&& tuple )
	: set_( true )
	, val_( std::move( get_tuple_element< T >( tuple ) ) )
	{ }

	template<
		typename Tuple,
		typename std::enable_if<
			tuple_arguments< Tuple >
				::template index_of< T >::value == -1
		>::type* = nullptr
	>
	single_option( Tuple&& tuple )
	: set_( false )
	{ }

	single_option( T&& t )
	: set_( true )
	, val_( std::move( t ) )
	{ }

	single_option( const T& t )
	: set_( true )
	, val_( t )
	{ }

	single_option( this_type&& ref ) = default;
	single_option( const this_type& ref ) = default;

	this_type& operator=( this_type&& ) = default;
	this_type& operator=( const this_type& ) = default;

	bool has( ) const
	{
		return set_;
	}

	const T& get( ) const
	{
		return val_;
	}

	T move( )
	{
		return std::move( val_ );
	}

private:
	bool set_;
	T val_;
};


template< typename... Args >
struct options;

template< typename First, typename... Rest >
struct options< First, Rest... >
: detail::single_option< First >
, options< Rest... >
{
	typedef options< First, Rest... > this_type;

	template< typename Tuple >
	options( Tuple&& tuple )
	: detail::single_option< First >( tuple )
	, options< Rest... >( tuple )
	{ }

	options( this_type&& options ) = default;
	options( const this_type& options ) = default;
};

template< >
struct options< >
{
	template< typename Tuple >
	options( Tuple&& tuple )
	{ }
};

} // namespace detail


template< typename... T >
class options
: public detail::options< T... >
{
public:
	static_assert(
		is_all_unique< T... >::value,
		"option types must be unique"
	);

	template< typename... Args >
	options( Args&&... args )
	: detail::options< T... >(
		std::make_tuple( std::forward< Args >( args )... ) )
	{ }

	template< typename U >
	typename std::enable_if<
		arguments< T... >::template index_of< U >::value != -1,
		bool
	>::type
	has( ) const
	{
		typedef detail::single_option< U > base_type;
		return static_cast< const base_type& >( *this ).has( );
	}

	template< typename U >
	typename std::enable_if<
		arguments< T... >::template index_of< U >::value != -1,
		U
	>::type
	get( ) const
	{
		typedef detail::single_option< U > base_type;
		return static_cast< const base_type& >( *this ).get( );
	}

	template< typename U >
	typename std::enable_if<
		arguments< T... >::template index_of< U >::value != -1,
		U
	>::type
	get( )
	{
		typedef detail::single_option< U > base_type;
		return static_cast< base_type& >( *this ).get( );
	}

	template< typename U >
	typename std::enable_if<
		arguments< T... >::template index_of< U >::value != -1,
		U
	>::type
	get( U _default ) const
	{
		typedef detail::single_option< U > base_type;
		return static_cast< const base_type& >( *this ).has( )
			?  static_cast< const base_type& >( *this ).get( )
			: _default;
	}

	template< typename U >
	typename std::enable_if<
		arguments< T... >::template index_of< U >::value != -1,
		U
	>::type
	get( U _default )
	{
		typedef detail::single_option< U > base_type;
		return static_cast< base_type& >( *this ).has( )
			? static_cast< base_type& >( *this ).get( )
			: _default;
	}

	template< typename U >
	typename std::enable_if<
		arguments< T... >::template index_of< U >::value != -1,
		U
	>::type
	move( )
	{
		typedef detail::single_option< U > base_type;
		return static_cast< base_type& >( *this ).move( );
	}

	template< typename U >
	typename std::enable_if<
		arguments< T... >::template index_of< U >::value != -1,
		U
	>::type
	move( U _default )
	{
		typedef detail::single_option< U > base_type;
		return static_cast< base_type& >( *this ).has( )
			? static_cast< base_type& >( *this ).move( )
			: _default;
	}
};

} // namespace q

#endif // LIBQ_OPTIONS_HPP
