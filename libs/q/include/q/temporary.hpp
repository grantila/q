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

#ifndef LIBQ_TEMPORARY_HPP
#define LIBQ_TEMPORARY_HPP

namespace q {

template< typename T >
struct is_temporary;

namespace detail {

template< typename T >
struct temporary;

template< typename T >
struct temporary_type
{
	typedef void type;
};
template< typename T >
struct temporary_type< const T& >
: temporary_type< T >
{ };
template< typename T >
struct temporary_type< T& >
: temporary_type< T >
{ };
template< typename T >
struct temporary_type< T&& >
: temporary_type< T >
{ };
template< typename T >
struct temporary_type< temporary< T > >
{
	typedef T type;
};

template< typename T >
struct temporary
{
	typedef T type;

	template< typename T_ >
	struct is
	: std::conditional<
		is_temporary< T_ >::value,
		typename std::is_same<
			T,
			typename std::decay<
				typename temporary_type< T_ >::type
			>::type
		>::type,
		typename std::is_same< T, T_ >::type
	>::type
	{ };

	template< typename T_ >
	struct is_decayed
	: std::conditional<
		is_temporary< T_ >::value,
		typename std::is_same<
			T,
			typename std::decay<
				typename temporary_type< T_ >::type
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

} // detail

template< typename T >
struct is_temporary
: std::false_type
{ };

template< typename T >
struct is_temporary< const T& >
: is_temporary< T >
{ };

template< typename T >
struct is_temporary< T& >
: is_temporary< T >
{ };

template< typename T >
struct is_temporary< T&& >
: is_temporary< T >
{ };

template< typename T >
struct is_temporary< detail::temporary< T > >
: std::true_type
{ };

template< typename T >
typename std::enable_if<
	is_temporary< T >::value,
	detail::temporary< T >
>::type
temporary( T&& t )
{
	return std::forward< T >( t );
}

template< typename T >
typename std::enable_if<
	!is_temporary< T >::value,
	detail::temporary< T >
>::type
temporary( T&& t )
{
	return detail::temporary< T >{ std::forward< T >( t ) };
}

template< typename T >
typename std::enable_if<
	is_temporary< T >::value,
	typename detail::temporary_type< T >::type
>::type
temporary_get( T&& t )
{
	return t.value;
}

template< typename T >
typename std::enable_if<
	!is_temporary< T >::value,
	T
>::type
temporary_get( T&& t )
{
	return t;
}

template< typename T >
typename std::enable_if<
	is_temporary< T >::value,
	typename detail::temporary_type< T >::type
>::type
temporary_forward( T&& t )
{
	return std::forward<
		typename detail::temporary_type< T >::type
	>( t.value );
}

template< typename T >
typename std::enable_if<
	!is_temporary< T >::value,
	T
>::type
temporary_forward( T&& t )
{
	return std::forward< T >( t );
}

} // namespace q

#endif // LIBQ_TEMPORARY_HPP
