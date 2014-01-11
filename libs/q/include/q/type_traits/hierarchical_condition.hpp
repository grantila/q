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

#ifndef LIBQ_TYPE_TRAITS_HIERARCHICAL_HPP
#define LIBQ_TYPE_TRAITS_HIERARCHICAL_HPP

namespace q {

template<
	template< typename > class Operator,
	template< typename, typename > class LogicOp,
	typename Nil,
	typename... T
>
struct hierarchically_satisfies_condition;

template<
	template< typename > class Operator,
	template< typename, typename > class LogicOp,
	typename Nil
>
struct hierarchically_generic_operator
{
	template< typename A, typename B >
	struct fold_type
	: LogicOp< Operator< A >, Operator< B > >
	{ };

	template< typename A, bool B >
	struct fold_type< A, std::integral_constant< bool, B > >
	: LogicOp< Operator< A >, bool_type< B > >
	{ };

	template< bool A, typename B >
	struct fold_type< std::integral_constant< bool, A >, B >
	: LogicOp< bool_type< A >, Operator< B > >
	{ };

	template< bool A, bool B >
	struct fold_type<
		std::integral_constant< bool, A >,
		std::integral_constant< bool, B >
	>
	: LogicOp< bool_type< A >, bool_type< B > >
	{ };

	template< typename... A, typename B >
	struct fold_type< std::tuple< A... >, B >
	: hierarchically_satisfies_condition< Operator, LogicOp, Nil, A..., B >
	{ };

	template< typename... A, bool B >
	struct fold_type<
		std::tuple< A... >,
		std::integral_constant< bool, B >
	>
	: hierarchically_satisfies_condition<
		Operator, LogicOp, Nil, A..., bool_type< B >
	>
	{ };

	template< bool A, typename... B >
	struct fold_type<
		std::integral_constant< bool, A >,
		std::tuple< B... >
	>
	: hierarchically_satisfies_condition<
		Operator, LogicOp, Nil, bool_type< A >, B...
	>
	{ };

	template< typename... A, typename... B >
	struct fold_type<
		std::tuple< A... >,
		std::tuple< B... >
	>
	: hierarchically_satisfies_condition<
		Operator, LogicOp, Nil, A..., B...
	>
	{ };
};

template<
	template< typename > class Operator,
	template< typename, typename > class LogicOp,
	typename Nil,
	typename... T
>
struct hierarchically_satisfies_condition
: fold<
	q::arguments< T... >,
	hierarchically_generic_operator<
		Operator, LogicOp, Nil
	>::template fold_type,
	Nil
>
{ };

template< template< typename > class Operator, typename... T >
struct hierarchically_satisfies_all_conditions
: hierarchically_satisfies_condition<
	Operator,
	logic_and,
	std::true_type,
	T...
>
{ };

template< template< typename > class Operator, typename... T >
struct hierarchically_satisfies_any_condition
: hierarchically_satisfies_condition<
	Operator,
	logic_or,
	std::false_type,
	T...
>
{ };

} // namespace q

#endif // LIBQ_TYPE_TRAITS_HIERARCHICAL_HPP
