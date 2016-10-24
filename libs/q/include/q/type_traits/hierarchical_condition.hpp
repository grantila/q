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
	: LogicOp< Operator< A >, bool_type_t< B > >
	{ };

	template< bool A, typename B >
	struct fold_type< std::integral_constant< bool, A >, B >
	: LogicOp< bool_type_t< A >, Operator< B > >
	{ };

	template< bool A, bool B >
	struct fold_type<
		std::integral_constant< bool, A >,
		std::integral_constant< bool, B >
	>
	: LogicOp< bool_type_t< A >, bool_type_t< B > >
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
		Operator, LogicOp, Nil, A..., bool_type_t< B >
	>
	{ };

	template< bool A, typename... B >
	struct fold_type<
		std::integral_constant< bool, A >,
		std::tuple< B... >
	>
	: hierarchically_satisfies_condition<
		Operator, LogicOp, Nil, bool_type_t< A >, B...
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
{
	typedef fold_t<
		q::arguments< T... >,
		hierarchically_generic_operator<
			Operator, LogicOp, Nil
		>::template fold_type,
		Nil
	> type;
};

template< template< typename > class Operator, typename... T >
struct hierarchically_satisfies_all_conditions
{
	typedef typename hierarchically_satisfies_condition<
		Operator,
		logic_and,
		std::true_type,
		T...
	>::type type;
};

template< template< typename > class Operator, typename... T >
struct hierarchically_satisfies_any_condition
{
	typedef typename hierarchically_satisfies_condition<
		Operator,
		logic_or,
		std::false_type,
		T...
	>::type type;
};

// Type aliases

template<
	template< typename > class Operator,
	template< typename, typename > class LogicOp,
	typename Nil,
	typename... T
>
using hierarchically_satisfies_condition_t =
	typename hierarchically_satisfies_condition<
		Operator, LogicOp, Nil, T...
	>::type;

template<
	template< typename > class Operator,
	typename... T
>
using hierarchically_satisfies_all_conditions_t =
	typename hierarchically_satisfies_all_conditions<
		Operator, T...
	>::type;

template<
	template< typename > class Operator,
	typename... T
>
using hierarchically_satisfies_any_condition_t =
	typename hierarchically_satisfies_any_condition<
		Operator, T...
	>::type;

} // namespace q

#endif // LIBQ_TYPE_TRAITS_HIERARCHICAL_HPP
