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

#ifndef LIBQ_TYPE_TRAITS_FOLD_HPP
#define LIBQ_TYPE_TRAITS_FOLD_HPP

namespace q {

namespace detail {

template<
	typename List,
	template< typename, typename > class Fold,
	class Nil
>
struct fold;

template<
	typename... Elements,
	template< typename, typename > class Fold,
	class Nil
>
struct fold<
	q::arguments< Elements... >,
	Fold,
	Nil
>
{
	typedef typename Fold<
		typename q::arguments< Elements... >::first_type,
		typename fold<
			typename q::arguments< Elements... >::rest_arguments,
			Fold,
			Nil
		>::type
	>::type type;
};

template<
	template< typename, typename > class Fold,
	class Nil
>
struct fold< q::arguments< >, Fold, Nil >
{
	typedef Nil type;
};

} // namespace detail

/**
 * q::fold is a meta class for folding types wrapped in a q::arguments using a
 * custom Fold meta function to fold two types, and a Nil type as the last
 * elemenent in the list.
 */
template<
	typename Elements,
	template< typename, typename > class Fold,
	class Nil
>
struct fold
{
	typedef typename detail::fold< Elements, Fold, Nil >::type type;
};

template<
	typename Elements,
	template< typename, typename > class Fold,
	class Nil
>
using fold_t = typename fold< Elements, Fold, Nil >::type;

} // namespace q

#endif // LIBQ_TYPE_TRAITS_FOLD_HPP
