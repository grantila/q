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
#include <q/bit_flags.hpp>

namespace q {

template< typename T >
struct required
{
	typedef T type;
};

namespace detail {

template< typename T >
struct _is_required
: std::false_type
{ };

template< typename T >
struct _is_required< ::q::required< T > >
: std::true_type
{ };

} // namespace detail

template< typename T >
struct is_required
: detail::_is_required< typename std::decay< T >::type >
{ };

template< typename T >
struct strip_required
{
	typedef T type;
};

template< typename T >
struct strip_required< required< T > >
{
	typedef T type;
};

namespace detail {

template< typename T >
struct single_option
{
	typedef single_option< T > this_type;

	single_option( )
	{ }

	template<
		typename Tuple,
		typename std::enable_if<
			tuple_arguments_t< typename std::decay< Tuple >::type >
				::template index_of< T >::value != -1
		>::type* = nullptr
	>
	single_option( Tuple&& tuple )
	: val_( std::move( get_tuple_element< T >( tuple ) ) )
	{ }

	template<
		typename Tuple,
		typename std::enable_if<
			tuple_arguments_t< typename std::decay< Tuple >::type >
				::template index_of< T >::value == -1
		>::type* = nullptr
	>
	single_option( Tuple&& tuple )
	{ }

	single_option( T&& t )
	: val_( std::move( t ) )
	{ }

	single_option( const T& t )
	: val_( t )
	{ }

	single_option( this_type&& ref ) = default;
	single_option( const this_type& ref ) = default;

	this_type& operator=( this_type&& ) = default;
	this_type& operator=( const this_type& ) = default;

	const T& get( ) const
	{
		return val_;
	}

	T move( )
	{
		return std::move( val_ );
	}

private:
	T val_;
};


template< typename... Args >
struct options;

template< typename First, typename... Rest >
struct options< First, Rest... >
: detail::single_option< First >
, options< Rest... >
{
	typedef options<  First, Rest... > this_type;

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
: public detail::options< typename strip_required< T >::type... >
{
public:
	typedef arguments< typename strip_required< T >::type... >
		stripped_arguments;
	typedef typename arguments< T... >
		::template filter< is_required >::type
		::template map< strip_required >::type
		::template map< std::decay >::type
		required_arguments;

	typedef bit_flags_of_types< typename strip_required< T >::type... >
		bit_flags_type;

	static_assert(
		are_all_unique< typename strip_required< T >::type... >::value,
		"option types must be unique"
	);

	/**
	 * Generic constructor for any type of input, as long as the input
	 * types match the options' types.
	 */
	template<
		typename... Args,
		typename = typename std::enable_if<
			argument_contains_all<
				arguments<
					typename strip_required< T >::type...
				>,
				arguments<
					typename std::decay< Args >::type...
				>
			>::value
		>::type
	>
	options( Args&&... args )
	: detail::options< typename strip_required< T >::type... >(
		std::make_tuple( std::forward< Args >( args )... ) )
	{
		static_assert(
			arguments< typename std::decay< Args >::type... >
			::template contains_all< required_arguments >::value,
			"All required options not provided"
		);

		set_.template set_by_type<
			typename std::decay< Args >::type...
		>( );
	}

	options( options< T... >&& ) = default;
	options( const options< T... >& ) = default;

	template< typename U >
	typename std::enable_if<
		stripped_arguments::template index_of< U >::value != -1,
		bool
	>::type
	has( ) const
	{
		return set_.template is_set_by_type< U >( );
	}

	template< typename U >
	typename std::enable_if<
		stripped_arguments::template index_of< U >::value != -1,
		U
	>::type
	get( ) const
	{
		typedef detail::single_option< U > base_type;
		return static_cast< const base_type& >( *this ).get( );
	}

	template< typename U >
	typename std::enable_if<
		stripped_arguments::template index_of< U >::value != -1,
		U
	>::type
	get( )
	{
		typedef detail::single_option< U > base_type;
		return static_cast< base_type& >( *this ).get( );
	}

	template< typename U >
	typename std::enable_if<
		stripped_arguments::template index_of< U >::value != -1,
		U
	>::type
	get( U _default ) const
	{
		typedef detail::single_option< U > base_type;
		return has< U >( )
			?  static_cast< const base_type& >( *this ).get( )
			: _default;
	}

	template< typename U >
	typename std::enable_if<
		stripped_arguments::template index_of< U >::value != -1,
		U
	>::type
	get( U _default )
	{
		typedef detail::single_option< U > base_type;
		return has< U >( )
			? static_cast< base_type& >( *this ).get( )
			: _default;
	}

	template< typename U >
	typename std::enable_if<
		stripped_arguments::template index_of< U >::value != -1,
		U
	>::type
	move( )
	{
		typedef detail::single_option< U > base_type;
		return static_cast< base_type& >( *this ).move( );
	}

	template< typename U >
	typename std::enable_if<
		stripped_arguments::template index_of< U >::value != -1,
		U
	>::type
	move( U _default )
	{
		typedef detail::single_option< U > base_type;
		return has< U >( )
			? static_cast< base_type& >( *this ).move( )
			: _default;
	}

private:
	bit_flags_type set_;
};

} // namespace q

#endif // LIBQ_OPTIONS_HPP
