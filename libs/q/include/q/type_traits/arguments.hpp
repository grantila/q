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

template< typename From, typename To >
struct is_argument_same_or_convertible_incl_void;

/**
 * Determines if the type T exists as any of the types in the q::arguments
 * Arguments.
 * Equals either std::true_type or std::false_type.
 */
template< typename Arguments, typename T >
struct argument_contains;

/**
 * Determines if all the types in the q::arguments Subset is found in the
 * q::arguments Superset.
 * Equals either std::true_type or std::false_type.
 */
template< typename Superset, typename Subset >
struct argument_contains_all;

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

	template< typename... T >
	struct identity
	{
		typedef arguments< T... > type;
	};

	template< typename... T >
	using identity_t = typename identity< T... >::type;

	template< template< typename... > class T >
	struct apply
	{
		typedef T< Args... > type;
	};

	template< template< typename... > class T >
	using apply_t = typename apply< T >::type;

	typedef typename detail::variadic_rest< Args... > rest;

	typedef typename rest::first_type first_type;
	typedef typename rest::template apply_rest< identity >::type::type
		rest_arguments;

	typedef typename apply< std::tuple >::type tuple_type;

	typedef std::integral_constant< std::size_t, sizeof...( Args ) > size;

	typedef std::false_type empty;

	typedef bool_type<
		size::value == 1
		and
		std::is_void< first_type >::value
	> empty_or_void;

	typedef bool_type<
		size::value == 1
		and
		(
			std::is_void< first_type >::value
			or
			std::is_same<
				typename std::decay< first_type >::type,
				void_t
			>::value
		)
	> empty_or_voidish;

#	ifdef LIBQ_WITH_CPP14

	static constexpr std::size_t size_v = size::value;
	static constexpr bool empty_v = false;
	static constexpr bool empty_or_void_v = empty_or_void::value;
	static constexpr bool empty_or_voidish_v = empty_or_voidish::value;

#	endif // LIBQ_WITH_CPP14

	// TODO: Why did I write this?
	template< template< template< typename... > class > class T >
	struct convert_in_tuple
	{
		typedef typename T<std::tuple>::type type;
	};

	template< typename... Before >
	struct prepend
	{
		typedef arguments< Before..., Args... > type;
	};

	template< typename... After >
	struct append
	{
		typedef arguments< Args..., After... > type;
	};

	template< typename T, std::intptr_t Offset = 0 >
	struct index_of
	: public rest_arguments::template index_of< T, Offset + 1 >
	{ };

	template< std::intptr_t Offset >
	struct index_of< first_type, Offset >
	: public std::integral_constant< std::intptr_t, Offset >
	{ };

	template< typename T >
	struct has
	: bool_type< index_of< T >::value != -1 >
	{ };

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

	template< typename T >
	struct is_convertible_to_incl_void
	: is_argument_same_or_convertible_incl_void< this_type, T >
	{ };

#	ifdef LIBQ_WITH_CPP14

	template< typename T >
	static constexpr bool is_convertible_to_v =
		is_convertible_to< T >::value;

	template< typename T >
	static constexpr bool is_convertible_to_incl_void_v =
		is_convertible_to_incl_void< T >::value;

#	endif // LIBQ_WITH_CPP14


	/**
	 * map each argument with a type T and return a new q::arguments.
	 *
	 * Example:
	 *   Given q::arguments< int, char > mapped to T returns:
	 *   q::arguments< T< int >, T< char > >
	 *
	 *   More precisely:
	 *   q::arguments< int, char >::template map< T >::type
	 *   // -> q::arguments< T< int >, T< char > >
	 */
	template< template< typename > class T >
	struct map
	{
		typedef arguments< typename T< Args >::type... > type;
	};

	/**
	 * filter each arguments through T which is supposed to resolve to a
	 * std::true_type or std::false_type. The resulting type is a (possibly
	 * shorter) q::arguments with only the types matching T.
	 */
	template< template< typename > class T >
	struct filter
	{
		typedef typename std::conditional<
			T< first_type >::value,
			typename rest_arguments
				::template filter< T >::type
				::template prepend< first_type >::type,
			typename rest_arguments
				::template filter< T >::type
		>::type type;
	};

	template< typename Arguments >
	struct _contains_all
	{
		typedef argument_contains_all< this_type, Arguments > type;
	};

	template< typename Arguments >
	struct contains_all
	: _contains_all< Arguments >::type
	{ };

	template< typename... T >
	struct contains
	: contains_all< arguments< T... > >
	{ };
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

	template< template< typename... > class T >
	using apply_t = typename apply< T >::type;

	typedef typename apply< std::tuple >::type tuple_type;

	typedef std::integral_constant< std::size_t, 0 > size;

	typedef std::true_type empty;

	typedef std::true_type empty_or_void;
	typedef std::true_type empty_or_voidish;

#	ifdef LIBQ_WITH_CPP14

	static constexpr std::size_t size_v = size::value;
	static constexpr bool empty_v = true;
	static constexpr bool empty_or_void_v = true;
	static constexpr bool empty_or_voidish_v = true;

#	endif // LIBQ_WITH_CPP14

	template< typename... Before >
	struct prepend
	{
		typedef arguments< Before... > type;
	};

	template< typename... After >
	struct append
	{
		typedef arguments< After... > type;
	};

	template< typename T, std::intptr_t = 0 >
	struct index_of
	: public std::integral_constant< std::intptr_t, -1 >
	{ };

	template< typename T >
	struct has
	: std::false_type
	{ };

	template< typename T >
	struct equals
	: is_argument_same< this_type, T >
	{ };

	template< typename T >
	struct is_convertible_to
	: is_argument_same_or_convertible< this_type, T >
	{ };

	template< typename T >
	struct is_convertible_to_incl_void
	: is_argument_same_or_convertible_incl_void< this_type, T >
	{ };

	template< template< typename > class T >
	struct map
	{
		typedef arguments< > type;
	};

	template< template< typename > class T >
	struct filter
	{
		typedef arguments< > type;
	};

	template< typename Arguments >
	struct contains_all
	: Arguments::empty
	{ };

	template< typename... T >
	struct contains
	: contains_all< arguments< T... > >
	{ };
};

namespace detail {

template< typename T >
struct tuple_arguments
{
	typedef arguments< T > type;
};

template< >
struct tuple_arguments< void >
{
	typedef arguments< > type;
};

template< typename... Args >
struct tuple_arguments< const std::tuple< Args... >& >
: public tuple_arguments< std::tuple< Args... > >
{ };

template< typename... Args >
struct tuple_arguments< std::tuple< Args... >&& >
: public tuple_arguments< std::tuple< Args... > >
{ };

template< typename... Args >
struct tuple_arguments< std::tuple< Args... > >
{
	typedef arguments< Args... > type;
};

template< template< typename... > class Predicate, size_t, typename... >
struct is_one_argument_and
: std::false_type
{ };

template< template< typename... > class Predicate, typename Arg >
struct is_one_argument_and< Predicate, 1, Arg >
: bool_type< Predicate< Arg >::value >
{ };

} // namespace detail

template< template< typename... > class Predicate, typename... Args >
struct is_one_argument_and
: detail::is_one_argument_and< Predicate, sizeof...( Args ), Args... >
{ };

} // namespace q

#endif // LIBQ_TYPE_TRAITS_ARGUMENTS_HPP
