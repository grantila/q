/*
 * Copyright 2016 Gustaf Räntilä
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

#ifndef LIBQ_RX_CORE_OPERATORS_HPP
#define LIBQ_RX_CORE_OPERATORS_HPP

#include <system_error>
#include <bitset>
#include <typeindex>
#include <vector>

#if LIBQ_WITH_CPP17
#	include <optional>
#	include <string_view>
#	include <variant>
#endif // LIBQ_WITH_CPP17

namespace q { namespace rx {

struct fake_operator
{
	std::size_t operator( )( int );
};

template< typename T >
struct is_fake_operator
: std::false_type
{ };

template< >
struct is_fake_operator< fake_operator >
: std::true_type
{ };

template< typename T >
constexpr bool is_fake_operator_v = is_fake_operator< T >::value;

template< typename T > struct has_hash : std::false_type { };

template< typename T > struct has_hash< T* > : std::true_type { };

template< > struct has_hash< bool > : std::true_type { };
template< > struct has_hash< char > : std::true_type { };
template< > struct has_hash< signed char > : std::true_type { };
template< > struct has_hash< unsigned char > : std::true_type { };
template< > struct has_hash< char16_t > : std::true_type { };
template< > struct has_hash< char32_t > : std::true_type { };
template< > struct has_hash< wchar_t > : std::true_type { };
template< > struct has_hash< short > : std::true_type { };
template< > struct has_hash< unsigned short > : std::true_type { };
template< > struct has_hash< int > : std::true_type { };
template< > struct has_hash< unsigned int > : std::true_type { };
template< > struct has_hash< long > : std::true_type { };
template< > struct has_hash< long long > : std::true_type { };
template< > struct has_hash< unsigned long > : std::true_type { };
template< > struct has_hash< unsigned long long > : std::true_type { };
template< > struct has_hash< float > : std::true_type { };
template< > struct has_hash< double > : std::true_type { };
template< > struct has_hash< long double > : std::true_type { };

template< > struct has_hash< std::string > : std::true_type { };
template< > struct has_hash< std::u16string > : std::true_type { };
template< > struct has_hash< std::u32string > : std::true_type { };
template< > struct has_hash< std::wstring > : std::true_type { };
template< > struct has_hash< std::error_code > : std::true_type { };
template< std::size_t I > struct has_hash< std::bitset< I > >
: std::true_type
{ };
template< typename T > struct has_hash< std::unique_ptr< T > >
: has_hash< T >
{ };
template< typename T > struct has_hash< std::shared_ptr< T > >
: has_hash< T >
{ };
template< > struct has_hash< std::type_index > : std::true_type { };
template< > struct has_hash< std::vector< bool > > : std::true_type { };
template< > struct has_hash< std::thread::id > : std::true_type { };

#if LIBQ_WITH_CPP17

template< typename T > struct has_hash< std::optional< T > >
: has_hash< T >
{ };
template< typename... T > struct has_hash< std::variant< T... > >
: hierarchically_satisfies_all_conditions< has_hash, T... >
{ };
template< > struct has_hash< std::string_view > : std::true_type { };
template< > struct has_hash< std::wstring_view > : std::true_type { };
template< > struct has_hash< std::u16string_view > : std::true_type { };
template< > struct has_hash< std::u32string_view > : std::true_type { };

#endif // LIBQ_WITH_CPP17

namespace detail {

template< typename T >
struct has_less_than_helper
{
	template< typename U >
	static std::true_type has_less_than(
		decltype( std::declval< U >( ) < std::declval< U >( ) )* );

	template< typename U >
	static std::false_type has_less_than( ... );

	typedef decltype( has_less_than< T >( nullptr ) ) has;
};

template<
	typename Operator,
	bool Unary = q::arity_of_v< decltype( &Operator::operator( ) ) > == 1
>
using is_unary_operator = bool_type< Unary >;

} // namespace detail

template< typename T >
struct has_less_than
: detail::has_less_than_helper< T >::has
{ };

/**
 * should_hash is a bool type which results to true for types known to be
 * hashable, and also true for other types if they are not less-than comparable,
 * and false for other types (that are less-than comparable).
 */
template< typename T, typename Operator, bool HasHash = has_hash< T >::value >
struct should_hash;

template< typename T, typename Operator >
struct should_hash< T, Operator, true >
: q::bool_type<
	is_fake_operator_v< Operator >
	or
	q::arity_of_v< decltype( &Operator::operator( ) ) > == 1
>
{ };

template< typename T, typename Operator >
struct should_hash< T, Operator, false >
: q::bool_type<
	(
		!is_fake_operator_v< Operator >
		and
		q::arity_of_v< decltype( &Operator::operator( ) ) > == 1
	)
	or
	!has_less_than< T >::value
>
{ };

template< typename T, typename Operator, bool HasHash = has_hash< T >::value >
constexpr bool should_hash_v = should_hash< T, Operator, HasHash >::value;

template< typename T, typename Hash >
struct specific_hasher
{
	typedef std::hash< Hash > type;
};

template< typename T >
struct specific_hasher< T, fake_operator >
{
	typedef std::hash< T > type;
};

template< typename T, typename Hash >
using specific_hasher_t = typename specific_hasher< T, Hash >::type;

template< typename T, typename Less >
struct specific_less
{
	typedef std::less< Less > type;
};

template< typename T >
struct specific_less< T, fake_operator >
{
	typedef std::less< T > type;
};

template< typename T, typename Less >
using specific_less_t = typename specific_less< T, Less >::type;

} } // namespace rx, namespace q

#endif // LIBQ_RX_CORE_OPERATORS_HPP
