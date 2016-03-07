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

template< typename T >
auto mul( T multiplier )
{
	return [ multiplier ]( auto number ) -> T
	{
		return static_cast< T >( number * multiplier );
	};
}

template< typename T >
auto div( T denominator )
{
	return [ denominator ]( auto numerator ) -> T
	{
		return static_cast< T >( numerator / denominator );
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

} // namespace detail

template< typename T >
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

template< typename T >
auto fast_div_pi( T&& t )
{
	typedef typename std::decay< T >::type type;

	auto is_too_large =
		static_cast< std::make_unsigned_t< type > >( t ) ^
		( detail::fast_pi_order< type > << 1 );

	return is_too_large
		? ( t * fast_pi.den ) / fast_pi.num
		: ( t / fast_pi.num ) * fast_pi.den;
}

} } } // namespace f, namespace rx, namespace q

#endif // LIBQ_RX_FUNCTIONAL_MATH_HPP
