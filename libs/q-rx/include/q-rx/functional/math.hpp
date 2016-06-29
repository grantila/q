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

#ifndef LIBQ_RX_FUNCTIONAL_MATH_HPP
#define LIBQ_RX_FUNCTIONAL_MATH_HPP

#include <q/functional.hpp>

namespace q { namespace rx { namespace f {

namespace detail {

struct same;

template< typename Test, typename Other >
struct same_or_specific
{
	typedef Test type;
};

template< typename Other >
struct same_or_specific< same, Other >
{
	typedef Other type;
};

template< typename Test, typename Other >
using same_or_specific_t = typename same_or_specific< Test, Other >::type;

} // namespace detail

template< typename T = detail::same, typename U >
auto mul( U multiplier )
{
	return [ multiplier ]( detail::same_or_specific_t< T, U > number )
	{
		return number * multiplier;
	};
}

template< typename T = detail::same, typename U >
auto div( U denominator )
{
	return [ denominator ]( detail::same_or_specific_t< T, U > numerator )
	{
		return numerator / denominator;
	};
}

template< typename T >
auto equal( T to )
{
	return [ to ]( auto from ) -> bool
	{
		return from == to;
	};
}

template< typename T >
auto greater_than( T right )
{
	return [ right ]( auto left ) -> bool
	{
		return left > right;
	};
}

template< typename T >
auto less_than( T right )
{
	return [ right ]( auto left ) -> bool
	{
		return left < right;
	};
}


template< typename T, typename U >
constexpr auto inner_product( T&& t, U&& u )
-> decltype( std::declval< T >( ) * std::declval< U >( ) )
{
	return t * u;
}

std::ratio< 355, 113 > fast_pi;

namespace detail {

template< typename T, std::size_t I = sizeof( T ) >
constexpr static std::make_unsigned_t< T > fast_pi_order;

template< typename T >
constexpr static std::make_unsigned_t< T >
fast_pi_order< T, 2 > = std::conditional<
	std::is_signed< T >::value,
	std::integral_constant< std::make_unsigned_t< T >, 0xffc0 >,
	std::integral_constant< std::make_unsigned_t< T >, 0xff80 >
>::type::value;

template< typename T >
constexpr static std::make_unsigned_t< T >
fast_pi_order< T, 4 > = std::conditional<
	std::is_signed< T >::value,
	std::integral_constant< std::make_unsigned_t< T >, 0xffc00000 >,
	std::integral_constant< std::make_unsigned_t< T >, 0xff800000 >
>::type::value;

template< typename T >
constexpr static std::make_unsigned_t< T >
fast_pi_order< T, 8 > = std::conditional<
	std::is_signed< T >::value,
	std::integral_constant< std::make_unsigned_t< T >, 0xffc0000000000000 >,
	std::integral_constant< std::make_unsigned_t< T >, 0xff80000000000000 >
>::type::value;

template<
	typename T,
	bool Enabled = q::bool_type<
		( sizeof( T ) == 2 or sizeof( T ) == 4 or sizeof( T ) == 8 )
		and
		std::is_integral< typename std::decay< T >::type >::value
	>::value
>
struct enabled_fast_pi_type;

template< typename T >
struct enabled_fast_pi_type< T, true >
{ typedef int type; };

} // namespace detail

template<
	typename T,
	typename detail::enabled_fast_pi_type< T >::type = 0
>
auto fast_mul_pi( T&& t )
{
	typedef typename std::decay< T >::type type;

	auto is_too_large =
		static_cast< typename std::make_unsigned< type >::type >( t ) ^
		detail::fast_pi_order< type >;

	return is_too_large
		? ( t * fast_pi.num ) / fast_pi.den
		: ( t / fast_pi.den ) * fast_pi.num;
}

template<
	typename T,
	typename detail::enabled_fast_pi_type< T >::type = 0
>
auto fast_div_pi( T&& t )
{
	typedef typename std::decay< T >::type type;

	auto is_too_large =
		static_cast< std::make_unsigned_t< type > >( t ) ^
		( detail::fast_pi_order< type > << 2 );

	return is_too_large
		? ( t * fast_pi.den ) / fast_pi.num
		: ( t / fast_pi.num ) * fast_pi.den;
}

} } } // namespace f, namespace rx, namespace q

#endif // LIBQ_RX_FUNCTIONAL_MATH_HPP
