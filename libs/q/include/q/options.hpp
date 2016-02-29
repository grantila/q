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
struct all_unique;

template< typename First, typename... Rest >
struct all_unique< First, Rest... >
: ::q::bool_type<
	arguments< Rest... >
		::template filter< same< First >::template as >
		::type::size::value == 0
>
{ };

template< >
struct all_unique< >
: std::true_type
{ };

} // namespace detail

template< typename... Args >
struct options;

template< typename First, typename... Rest >
struct options< First, Rest... >
: detail::single_option< First >
, options< Rest... >
{
	typedef detail::single_option< First > first_option_type;
	typedef options< Rest... > rest_options_type;
	typedef options< First, Rest... > this_type;

	options( ) = default;
	options( this_type&& options ) = default;
	options( const this_type& options ) = default;

	options( First&& first, options< Rest... >&& rest )
	: detail::single_option< First >( std::move( first ) )
	, options< Rest... >( std::move( rest ) )
	{ }

	options( const First& first, options< Rest... >&& rest )
	: detail::single_option< First >( first )
	, options< Rest... >( std::move( rest ) )
	{ }

	this_type& operator=( this_type&& ref ) = default;
	this_type& operator=( const this_type& ref ) = default;

	template< typename... Args >
	struct contains
	: arguments< First, Rest... >::template contains< Args... >
	{ };

	/**
	 * Moves another options into this one. Only available if all types in
	 * the source options exist in this options.
	 */
	template< typename... Args >
	typename std::enable_if< contains< Args... >::value, this_type >::type&
	operator=( options< Args... >&& ref )
	{
		_assign( std::move( ref ) );
		return *this;
	}

	/**
	 * Copies another options into this one. Only available if all types in
	 * the source options exist in this options.
	 */
	template< typename... Args >
	typename std::enable_if< contains< Args... >::value, this_type >::type&
	operator=( const options< Args... >& ref )
	{
		options< Args... > copy = ref;
		return operator=( std::move( copy ) );
	}

	template< typename T >
	typename std::enable_if<
		std::is_base_of< T, this_type >::value,
		T&&
	>::type
	move_type( )
	{
		return std::move( static_cast< T& >( *this ) );
	}

	template< typename T >
	typename std::enable_if<
		!std::is_base_of< T, this_type >::value,
		T&&
	>::type
	move_type( )
	{
		T tmp;
		return std::move( tmp );
	}

	template< typename... Args >
	void _assign( options< Args... >&& ref )
	{
		// Move first type into this
		static_cast< first_option_type& >( *this ) = std::move(
			ref.template move_type< first_option_type >( ) );

		// Move the rest of the types recursively
		static_cast< rest_options_type& >( *this )._assign(
			std::move( ref ) );
	}
};

template< >
struct options< >
{
	template< typename... Args >
	void _assign( options< Args... >&& ref )
	{ }
};

namespace detail {

template< typename... Args >
struct options_builder
: ::q::options< Args... >
{
	options_builder( ) = default;
	options_builder( options_builder&& ) = default;
	options_builder( const options_builder& ) = default;

	template< typename T, typename... Rest >
	options_builder( T&& t, options_builder< Rest... >&& rest )
	: ::q::options< T, Rest... >( std::forward( t ), std::move( rest ) )
	{ }

	/**
	 * Pipe (append) more options. The function will only allow to append
	 * one of a certain type, and this operator is not available for types
	 * already appended.
	 */
	template< typename T >
	typename std::enable_if<
		arguments< Args... >
			::template filter< same< T >::template as >
			::type::size::value == 0,
		options_builder< typename std::decay< T >::type, Args... >
	>::type
	operator|( T&& t )
	{
		typedef options_builder<
			typename std::decay< T >::type, Args...
		> options_builder_type;

		return options_builder_type(
			std::move( t ), std::move( *this ) );
	}
};

template< typename First, typename... Rest >
struct options_builder< First, Rest... >
: ::q::options< First, Rest... >
{
	options_builder( First&& first, options_builder< Rest... >&& rest )
	: ::q::options< First, Rest... >( std::move( first ), std::move( rest ) )
	{ }

	options_builder( const First& first, options_builder< Rest... >&& rest )
	: ::q::options< First, Rest... >( first, std::move( rest ) )
	{ }

	/**
	 * Pipe (append) more options. The function will only allow to append
	 * one of a certain type, and this operator is not available for types
	 * already appended.
	 */
	template< typename T >
	typename std::enable_if<
		arguments< First, Rest... >
			::template filter< same< T >::template as >
			::type::size::value == 0,
		options_builder<
			typename std::decay< T >::type, First, Rest...
		>
	>::type
	operator|( T&& t )
	{
		typedef options_builder<
			typename std::decay< T >::type, First, Rest...
		> options_builder_type;

		return options_builder_type(
			std::forward< T >( t ), std::move( *this ) );
	}
};

} // namespace detail

static inline detail::options_builder< >
choose( ) {
	return detail::options_builder< >( );
}

template< typename T, typename Options >
typename std::enable_if<
	Options::template contains< T >::value,
	bool
>::type
has_option( const Options& opts )
{
	return static_cast< const detail::single_option< T >& >( opts ).has( );
}

template< typename T, typename Options >
typename std::enable_if<
	Options::template contains< T >::value,
	const T&
>::type
get_option( const Options& opts )
{
	return static_cast< const detail::single_option< T >& >( opts ).get( );
}

template< typename T, typename Options >
typename std::enable_if<
	Options::template contains< T >::value,
	T
>::type
move_option( Options& opts )
{
	return static_cast< detail::single_option< T >& >( opts ).move( );
}

} // namespace q

#endif // LIBQ_OPTIONS_HPP
