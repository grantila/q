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

#ifndef LIBQ_TYPE_TRAITS_CORE_HPP
#define LIBQ_TYPE_TRAITS_CORE_HPP

#include <ciso646>
#include <type_traits>
#include <tuple>
#include <memory>
#include <vector>

namespace q {
#define Q_FORWARD( value ) \
	::std::forward< \
		decltype( value ) \
	>( value )

template< typename T > void ignore_result( T&& ) { }
template< typename T > void ignore_parameter( T&& ) { }

struct void_t { };

namespace detail {

template< typename T >
T identity_fn( T&& );

template< typename T >
typename std::remove_reference< T >::type identity_fn_noref( T&& );

} // namespace detail

/**
 * Provides a functional operator which expands ::type from a meta-function
 */
template< template< typename > class T >
struct functional_type
{
	template< typename U >
	struct of
	: T< U >::type
	{ };
};

/**
 * q::bool_type is std::true_type if value is true, otherwise q::bool_type is
 * std::false_type.
 */
template< bool value >
struct bool_type
: std::integral_constant< bool, value >
{ };

/**
 * Negates the meaning of is_* meta classes, such as std::is_void< >.
 *
 * The usage to invert e.g. std::is_void< T > would be:
 * q::negate< std::is_void >::template of< T >
 *
 * @see q::isnt
 */
template< template< typename... > class T >
struct negate
{
	template< typename... Args >
	struct of
	: bool_type< !T< Args... >::value >
	{
		typedef bool_type< !T< Args... >::value > type;
	};
};

/**
 * Simplified and specialized version of q::negate when the arguments are
 * immediately available.
 *
 * Usage:
 *   q::isnt< std::is_void, T >
 */
template< template< typename... > class T, typename... Args >
struct isnt
: negate< T >::template of< Args... >::type
{
	typedef typename negate< T >::template of< Args... >::type type;
};

/**
 * Similar to std::is_same, although this one disregards constness and
 * references.
 */
template<
	typename A,
	typename B >
struct is_same_type
: std::is_same<
	typename std::remove_reference<
		typename std::decay< A >::type
	>::type,
	typename std::remove_reference<
		typename std::decay< B >::type
	>::type
>
{ };

/**
 * Composable is_same_type
 */
template< typename A >
struct same
{
	template< typename B >
	struct as
	: is_same_type< A, B >
	{ };
};

template< typename... T >
struct is_tuple
: std::false_type
{ };

template< typename... T >
struct is_tuple< std::tuple< T... > >
: std::true_type
{ };

template< typename Tuple >
struct is_empty_tuple
: std::false_type
{ };

template< >
struct is_empty_tuple< std::tuple< > >
: std::true_type
{ };

template< typename T >
struct is_copyable
: bool_type<
	std::is_copy_constructible< T >::value ||
	std::is_copy_assignable< T >::value
>
{ };
/* TODO: Implement for variadic templates
template< typename... T >
struct is_copyable< std::tuple< T... > >
: is_copyable<  >
{ };
*/


template< typename... T >
struct is_nothrow_default_constructible;

template< typename... T >
struct is_copy_constructible;

template< typename... T >
struct is_nothrow_copy_constructible;

template< typename... T >
struct is_copy_assignable;

template< typename... T >
struct is_move_constructible;

template< typename T >
struct is_nothrow_copyable
: bool_type<
	q::is_nothrow_copy_constructible< T >::value ||
	std::is_nothrow_copy_assignable< T >::value
>
{ };

template< typename T >
struct is_movable
: bool_type<
	q::is_move_constructible< T >::value ||
	std::is_move_assignable< T >::value
>
{ };

template< typename T >
struct is_nothrow_movable
: bool_type<
	std::is_nothrow_move_constructible< T >::value ||
	std::is_nothrow_move_assignable< T >::value
>
{ };

template< typename T >
struct is_copyable_or_movable
: bool_type<
	is_copyable< T >::value ||
	is_movable< T >::value
>
{ };

template< typename T >
struct is_nothrow_copyable_or_movable
: bool_type<
	is_nothrow_copyable< T >::value ||
	is_nothrow_movable< T >::value
>
{ };

template< typename T >
struct is_shared_pointer
: std::false_type
{ };

template< typename T >
struct is_shared_pointer< std::shared_ptr< T > >
: std::true_type
{ };

template< typename T >
struct is_unique_pointer
: std::false_type
{ };

template< typename T >
struct is_unique_pointer< std::unique_ptr< T > >
: std::true_type
{ };

template< typename T >
struct is_pointer_like
: bool_type<
	std::is_pointer< T >::value ||
	is_shared_pointer< T >::value ||
	is_unique_pointer< T >::value
>
{ };

template< typename T >
struct remove_cv_ref
{
	typedef typename std::remove_cv<
		typename std::remove_reference< T >::type
	>::type type;
};

template< typename T >
struct remove_rvalue_reference
{
	typedef T type;
};

template< typename T >
struct remove_rvalue_reference< T&& >
{
	typedef T type;
};

/**
 * This type is true only for char[N].
 */
template< typename T >
struct is_char_array
: public bool_type<
	std::is_array< typename remove_cv_ref< T >::type >::value &&
	std::is_same<
		typename remove_cv_ref<
			typename std::remove_extent<
				typename remove_cv_ref< T >::type
			>::type
		>::type,
		char
	>::value
>
{ };

/**
 * This type is true only for char*.
 */
template< typename T >
struct is_char_pointer
: public bool_type<
	std::is_pointer< typename remove_cv_ref< T >::type >::value &&
	std::is_same<
		typename remove_cv_ref<
			typename std::remove_pointer<
				typename remove_cv_ref< T >::type
			>::type
		>::type,
		char
	>::value
>
{ };

/**
 * This type is true for both char[N] and char*.
 */
template< typename T >
struct is_c_string
: public bool_type< is_char_array< T >::value or is_char_pointer< T >::value >
{ };

template< typename T >
struct is_vector
: std::false_type
{ };

template< typename T >
struct is_vector< std::vector< T > >
: std::true_type
{ };

template< typename Base, typename Derived >
struct is_base_of
: std::is_base_of< Base, Derived >
{ };

template< typename Base, typename Derived >
struct is_base_of< Base*, Derived* >
: std::is_base_of< Base, Derived >
{ };

template< typename... T >
struct not_implemented;

template< typename T >
typename std::decay< T >::type forward_decay( T&& v )
{
	return std::forward< T >( v );
}

template< typename... Args >
auto
forward_decay_as_tuple( Args&&... args )
-> decltype( std::make_tuple( forward_decay( std::forward< Args >( args ) )... ) )
{
	return std::make_tuple( forward_decay( std::forward< Args >( args ) )... );
}

template< typename... T >
struct decayed_tuple
{
	typedef std::tuple< typename std::decay< T >::type... > type;
};

template< typename T >
struct tuple_of
: std::tuple< T >
{
	typedef std::tuple< T > type;
};

template< >
struct tuple_of< void >
: std::tuple< >
{
	typedef std::tuple< > type;
};

template< typename... T >
struct tuple_of< std::tuple< T... > >
: std::tuple< T... >
{
	typedef std::tuple< T... > type;
};

// Get the same kind of integer (signed/unsigned) but of ptr_size
template<
	typename T,
	bool IsInteger = std::is_integral< T >::value,
	bool IsSigned = std::is_signed< T >::value
>
struct ptr_size_integer;

template< typename T >
struct ptr_size_integer< T, true, false >
{
	typedef std::uintptr_t type;
};

template< typename T >
struct ptr_size_integer< T, true, true >
{
	typedef std::intptr_t type;
};

template< typename T >
using ptr_size_integer_t = typename ptr_size_integer< T >::type;

template< typename T, typename... Args >
typename std::enable_if<
	!std::is_array< T >::value,
	std::unique_ptr< T >
>::type
make_unique( Args&&... args )
{
#ifdef LIBQ_WITH_CPP14
	return std::make_unique< T >( std::forward< Args >( args )... );
#else
	return std::unique_ptr< T >( new T( std::forward< Args >( args )... ) );
#endif
}

template< typename T >
typename std::enable_if<
	std::is_array< T >::value,
	std::unique_ptr< T >
>::type
make_unique( std::size_t size )
{
#if __cplusplus >= 201402L
	return std::make_unique< T >( size );
#else
	typedef typename std::remove_extent< T >::type U;
	return std::unique_ptr< T >( new U[ size ] );
#endif
}


} // namespace q

#endif // LIBQ_TYPE_TRAITS_CORE_HPP
