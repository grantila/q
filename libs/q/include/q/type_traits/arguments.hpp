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

#ifndef LIBQ_TYPE_TRAITS_ARGUMENTS_HPP
#define LIBQ_TYPE_TRAITS_ARGUMENTS_HPP

namespace q {

namespace detail {

template< typename First, typename... Rest >
struct variadic_rest
{
	typedef First first_type;

	template< template< typename... > class T >
	struct apply_rest
	{
		typedef T< Rest... > type;
	};

	typedef std::integral_constant< bool, ( sizeof...( Rest ) > 0 ) > has_rest;
};

} // namespace detail

/**
 * Determines if the arguments in a q::arguments (@a From) are the same as the
 * arguments in a q::arguments (@a To).
 */
template< typename From, typename To >
struct is_argument_same;

/**
 * Determines if the arguments in a q::arguments (@a From) are the same or
 * convertible into the arguments in a q::arguments (@a To).
 */
template< typename From, typename To >
struct is_argument_same_or_convertible;

/**
 * Generic variadic template argument generator which also provides simple
 * helpers for traversing and managing variadic templates.
 * 
 * @c first_type is the first argument type.
 * @c rest_arguments is an @c arguments<> with the rest of the arguments (i.e.
 *    the second and following arguments)
 * @c tuple_type is a @c std::tuple with the same arguments as this class.
 */
template< typename... Args >
struct arguments
{
	typedef arguments< Args... > this_type;

	template< template< typename... > class T >
	struct apply
	{
		typedef T< Args... > type;
	};

	template< typename... T >
	struct identity
	{
		typedef arguments< T... > type;
	};

	// TODO: Why did I write this?
	template< template< template< typename... > class > class T >
	struct convert_in_tuple
	{
		typedef typename T<std::tuple>::type type;
	};

	/**
	 * Given a q::arguments wrapped set of types T, checks whether the
	 * current arguments are individually equal to their corresponding
	 * position in T
	 */
	template< typename T >
	struct equals
	: is_argument_same< this_type, T >
	{ };

	/**
	 * Given a q::arguments wrapped set of types T, checks whether the
	 * current arguments are individually equal or convertible into their
	 * corresponding position in T
	 */
	template< typename T >
	struct is_convertible_to
	: is_argument_same_or_convertible< this_type, T >
	{ };

	typedef typename detail::variadic_rest< Args... > rest;

	typedef typename rest::first_type first_type;
	typedef typename rest::template apply_rest< identity >::type::type rest_arguments;

	typedef typename apply< std::tuple >::type tuple_type;

	typedef std::integral_constant< std::size_t, sizeof...( Args ) > size;
};

template< >
struct arguments< >
{
	typedef arguments< > this_type;

	template< template< typename... > class T >
	struct apply
	{
		typedef T< > type;
	};

	template< typename T >
	struct equals
	: is_argument_same< this_type, T >
	{ };

	template< typename T >
	struct is_convertible_to
	: is_argument_same_or_convertible< this_type, T >
	{ };

	typedef typename apply< std::tuple >::type tuple_type;

	typedef std::integral_constant< std::size_t, 0 > size;
};

namespace detail {

template< typename T >
struct tuple_arguments
: public arguments< T >
{ };

template< >
struct tuple_arguments< void >
: public arguments< >
{ };

template< typename... Args >
struct tuple_arguments< std::tuple< Args... >& >
: public tuple_arguments< std::tuple< Args... > >
{ };

template< typename... Args >
struct tuple_arguments< std::tuple< Args... > >
: public arguments< Args... >
{ };

} // namespace detail

} // namespace q

#endif // LIBQ_TYPE_TRAITS_ARGUMENTS_HPP
