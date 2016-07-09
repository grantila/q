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

#ifndef LIBQ_TYPE_TRAITS_ARGUMENTS_IMPL_HPP
#define LIBQ_TYPE_TRAITS_ARGUMENTS_IMPL_HPP

namespace q {

namespace detail {

template< typename Tuple, class Args >
struct tuple_convertible_to_arguments
: public std::true_type
{
//	forward_decay
	// TODO: Implement
};

} // namespace detail

template< typename A, typename B >
struct is_argument_same
: two_fold< A, B, q::logic_and, std::true_type, q::is_same_type, std::false_type >
{ };

template< typename A, typename B >
struct is_argument_same_or_convertible
: bool_type<
	( A::empty_or_void::value and B::empty_or_void::value )
	or
	two_fold<
		A,
		B,
		q::logic_and,
		std::true_type,
		q::is_convertible_to, std::false_type
	>::value
>
{ };

template< typename Superset, typename Subset >
struct argument_contains_all
: fold<
	Subset,
	generic_operator<
		Superset::template has, logic_and
	>::template fold_type,
	std::true_type
>
{ };




namespace detail {

template< typename A, typename B >
struct merge_two_arguments;

template< typename... A, typename... B >
struct merge_two_arguments< ::q::arguments< A... >, ::q::arguments< B... > >
: ::q::arguments< A..., B... >
{
	typedef ::q::arguments< A..., B... > type;
};

} // namespace detail

/**
 * Merges the inner arguments of the list of q::arguments Arguments and injects
 * in T.
 *
 * A typical usage is to merge multiple q::arguments into one.
 */
template< template< typename... > class T, typename... Arguments >
struct merge
{
	typedef typename fold<
		q::arguments< Arguments... >,
		detail::merge_two_arguments,
		q::arguments< >
	>::template apply< T >::type type;
};


template< typename... Args >
struct is_all_unique;

template< typename First, typename... Rest >
struct is_all_unique< First, Rest... >
: ::q::bool_type<
	arguments< Rest... >
		::template filter< same< First >::template as >
		::type::size::value == 0
	and
	is_all_unique< Rest... >::value
>
{ };

template< >
struct is_all_unique< >
: std::true_type
{ };


template< typename T, typename... Types >
typename std::enable_if<
	is_all_unique< Types... >::value,
	T&
>::type
get_tuple_element( std::tuple< Types... >& t )
{
#ifdef LIBQ_WITH_CPP14
	return std::get< T >( t );
#else
	typedef typename arguments< Types... >::template index_of< T > Index;
	static_assert( Index::value != -1, "No such type in the given tuple" );
	return std::get< Index::value >( t );
#endif
}

template< typename T, typename... Types >
typename std::enable_if<
	is_all_unique< Types... >::value,
	T&&
>::type
get_tuple_element( std::tuple< Types... >&& t )
{
#ifdef LIBQ_WITH_CPP14
	return std::get< T >( std::move( t ) );
#else
	typedef typename arguments< Types... >::template index_of< T > Index;
	static_assert( Index::value != -1, "No such type in the given tuple" );
	return std::get< Index::value >( std::move( t ) );
#endif
}

template< typename T, typename... Types >
typename std::enable_if<
	is_all_unique< Types... >::value,
	const T&
>::type
get_tuple_element( const std::tuple< Types... >& t )
{
#ifdef LIBQ_WITH_CPP14
	return std::get< T >( t );
#else
	typedef typename arguments< Types... >::template index_of< T > Index;
	static_assert( Index::value != -1, "No such type in the given tuple" );
	return std::get< Index::value >( t );
#endif
}

} // namespace q

#endif // LIBQ_TYPE_TRAITS_ARGUMENTS_IMPL_HPP
