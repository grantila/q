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
: two_fold< A, B, q::logic_and, std::true_type, q::is_convertible_to, std::false_type >
{ };


namespace detail
{

template< typename _T >
struct argument_contains_iterator
{
	typedef _T T;
	typedef std::false_type found;
};

template< typename _T, typename Iterator >
struct argument_contains_fun
{
	typedef typename Iterator::T T;
	typedef std::is_same< T, _T > is_current_same;
	typedef logic_or< is_current_same, typename Iterator::found > found;
	typedef argument_contains_fun< _T, Iterator > type;
};

template< typename _Superset >
struct argument_contains_all_iterator
{
	typedef _Superset Superset;
	typedef std::true_type found;
};

template< typename T, typename Iterator >
struct argument_contains_all_fun
{
	typedef typename Iterator::Superset Superset;
	typedef argument_contains< Superset, T > is_current_found;
	typedef logic_and< is_current_found, typename Iterator::found > found;
	typedef argument_contains_all_fun< T, Iterator > type;
};

} // namespace detail

template< typename... Args, typename T >
struct argument_contains< arguments< Args... >, T >
: fold<
	arguments< Args... >,
	detail::argument_contains_fun,
	detail::argument_contains_iterator< T >
>::found
{ };

template< typename Superset, typename Subset >
struct argument_contains_all
: fold<
	Subset,
	detail::argument_contains_all_fun,
	detail::argument_contains_all_iterator< Superset >
>::found
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

} // namespace q

#endif // LIBQ_TYPE_TRAITS_ARGUMENTS_IMPL_HPP
