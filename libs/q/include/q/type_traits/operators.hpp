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

#ifndef LIBQ_TYPE_TRAITS_OPERATORS_HPP
#define LIBQ_TYPE_TRAITS_OPERATORS_HPP

namespace q {

template< typename A, typename B >
struct logic_and
: q::bool_type< A::value && B::value >
{
	typedef std::true_type nil_type;
};

template< typename A, typename B >
struct logic_or
: q::bool_type< A::value || B::value >
{
	typedef std::false_type nil_type;
};

template< typename T, bool MetaValue = true >
struct wrapped_value
: T
{
	typedef q::bool_type< MetaValue > meta;
};

template< typename T >
struct is_wrapped_value
: std::false_type
{ };

template< typename T >
struct is_wrapped_value< wrapped_value< T > >
: std::true_type
{ };

template< typename A, typename B >
struct logic_eq
: wrapped_value< A,
	std::is_same< typename A::value_type, typename B::value_type >::value &&
	A::value == B::value
>
{ };

template< typename A, typename B, bool M >
struct logic_eq< wrapped_value< A, M >, B >
: wrapped_value< typename A::type,
	std::is_same< typename A::value_type, typename B::value_type >::value &&
	A::value == B::value &&
	M
>
{ };

template< typename A, typename B >
struct size_eq
: logic_eq< typename A::size, typename B::size >
{ };

template<
	template< typename > class Operator,
	template< typename, typename > class LogicOp
>
struct generic_operator
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
};

} // namespace q

#endif // LIBQ_TYPE_TRAITS_OPERATORS_HPP
