/*
 * Copyright 2015 Gustaf Räntilä
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

#ifndef LIBQ_SETDEFAULT_HPP
#define LIBQ_SETDEFAULT_HPP

#define Q_IS_SETDEFAULT_SAME( type1, type2 ) \
	::q::defaultable< \
		typename std::decay< type1 >::type \
	>::template is_decayed< type2 >::value

namespace q {

template< typename T >
struct is_set_default;

template< typename T >
struct defaultable;

namespace detail {

template< typename T >
struct set_default_type
{
	typedef void type;
};
template< typename T >
struct set_default_type< const T& >
: set_default_type< T >
{ };
template< typename T >
struct set_default_type< T& >
: set_default_type< T >
{ };
template< typename T >
struct set_default_type< T&& >
: set_default_type< T >
{ };
template< typename T >
struct set_default_type< defaultable< T > >
{
	typedef T type;
};

} // detail

template< typename T >
struct defaultable
{
	typedef T type;

	template< typename T_ >
	struct is
	: std::conditional<
		is_set_default< T_ >::value,
		typename std::is_same<
			T,
			typename std::decay<
				typename detail::set_default_type< T_ >::type
			>::type
		>::type,
		typename std::is_same< T, T_ >::type
	>::type
	{ };

	template< typename T_ >
	struct is_decayed
	: std::conditional<
		is_set_default< T_ >::value,
		typename std::is_same<
			T,
			typename std::decay<
				typename detail::set_default_type< T_ >::type
			>::type
		>::type,
		typename std::is_same<
			T,
			typename std::decay< T_ >::type
		>::type
	>::type
	{ };

	T value;
};

template< typename T >
struct is_set_default
: std::false_type
{ };

template< typename T >
struct is_set_default< const T& >
: is_set_default< T >
{ };

template< typename T >
struct is_set_default< T& >
: is_set_default< T >
{ };

template< typename T >
struct is_set_default< T&& >
: is_set_default< T >
{ };

template< typename T >
struct is_set_default< defaultable< T > >
: std::true_type
{ };

template< typename T >
typename std::enable_if<
	is_set_default< T >::value,
	defaultable< T >
>::type
set_default( T&& t )
{
	return std::forward< T >( t );
}

template< typename T >
typename std::enable_if<
	!is_set_default< T >::value,
	defaultable< typename std::decay< T >::type >
>::type
set_default( T&& t )
{
	return defaultable< typename std::decay< T >::type >
		{ std::forward< T >( t ) };
}

template< typename T >
typename std::enable_if<
	is_set_default< T >::value,
	typename detail::set_default_type< T >::type
>::type
set_default_get( T&& t )
{
	return t.value;
}

template< typename T >
typename std::enable_if<
	!is_set_default< T >::value,
	T
>::type
set_default_get( T&& t )
{
	return t;
}

template< typename T >
typename std::enable_if<
	is_set_default< T >::value,
	typename detail::set_default_type< T >::type
>::type
set_default_forward( T&& t )
{
	return std::forward<
		typename detail::set_default_type< T >::type
	>( t.value );
}

template< typename T >
typename std::enable_if<
	!is_set_default< T >::value,
	T
>::type
set_default_forward( T&& t )
{
	return std::forward< T >( t );
}

} // namespace q

#endif // LIBQ_SETDEFAULT_HPP
