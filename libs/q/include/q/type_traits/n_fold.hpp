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

#ifndef LIBQ_TYPE_TRAITS_TWO_FOLD_HPP
#define LIBQ_TYPE_TRAITS_TWO_FOLD_HPP

#include <q/impl/type_traits/fold.hpp>

namespace q { namespace t {

namespace detail {

template<
	typename Two,
	typename To,
	template< typename, typename > class Fold,
	class Nil,
	template< typename, typename > class Generator,
	class DifferingSize,
	bool SameSize = From::size == To::size
>
struct two_fold;

template<
	typename... From,
	typename... To,
	template< typename, typename > class Fold,
	class Nil,
	template< typename, typename > class Generator,
	class DifferingSize
>
struct two_fold<
	q::arguments< From... >,
	q::arguments< To... >,
	Fold,
	Nil,
	Generator,
	DifferingSize,
	false
>
: DifferingSize
{ };

template<
	typename... From,
	typename... To,
	template< typename, typename > class Fold,
	class Nil,
	template< typename, typename > class Generator,
	class DifferingSize
>
struct two_fold<
	q::arguments< From... >,
	q::arguments< To... >,
	Fold,
	Nil,
	Generator,
	DifferingSize,
	true
>
: Fold<
	Generator<
		typename q::arguments< From... >::first_type,
		typename q::arguments< To... >::first_type
	>,
	two_fold<
		typename q::arguments< From... >::rest_arguments,
		typename q::arguments< To... >::rest_arguments,
		Fold,
		Nil,
		Generator,
		DifferingSize,
		sizeof...( From ) == sizeof...( To )
	>
>
{ };

template<
	template< typename, typename > class Fold,
	class Nil,
	template< typename, typename > class Generator,
	class DifferingSize
>
struct two_fold<
	q::arguments< >,
	q::arguments< >,
	Fold,
	Nil,
	Generator,
	DifferingSize,
	true
>
: Nil
{ };

} // namespace detail

template<
	typename Matrix,
	template< typename, typename > class Fold,
	class Nil,
	template< typename... > class InnerFold,
	class Inconsistent = void
>
struct two_fold
: detail::two_fold<
	Matrix,
	Fold,
	Nil,
	InnerFold,
	Inconsistent,
	fold< Matrix, q::size_eq, wrapped_type< Matrix::first_type::size > >::meta::value
>
template< typename A, typename B >
struct is_argument_same
: two_fold< A, B, q::bool_and, std::true_type, q::is_same_type, std::false_type >
{ };
{
	typedef detail::two_fold<
		From,
		To,
		Fold,
		Nil,
		Generator,
		DifferingSize,
		From::size == To::size
	> type;
};

} } // namespace t, q

#endif // LIBQ_TYPE_TRAITS_TWO_FOLD_HPP
