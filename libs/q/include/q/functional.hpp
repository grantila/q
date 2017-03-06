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

#ifndef LIBQ_FUNCTIONAL_HPP
#define LIBQ_FUNCTIONAL_HPP

#include <iostream>
#include <functional>

#include <q/pp.hpp>
#include <q/type_traits.hpp>


#define Q_FUNCTIONTRAITS( Fn ) \
	::q::function_traits< Fn >

#define Q_RESULT_OF( Fn ) \
	typename ::q::function_traits< Fn >::result_type

#define Q_RESULT_OF_AS_ARGUMENT( Fn ) \
	::q::function_traits< Fn >::result_argument_type

#define Q_RESULT_OF_AS_TUPLE_TYPE( Fn ) \
	typename Q_RESULT_OF_AS_ARGUMENT( Fn )::tuple_type

#define Q_RESULT_OF_AS_ARGUMENT_TYPE( Fn ) \
	typename Q_RESULT_OF_AS_ARGUMENT( Fn )

#define Q_ARITY_OF( Fn ) \
	::q::function_traits< Fn >::arity::value

#define Q_ARGUMENTS_OF( Fn ) \
	typename ::q::function_traits< Fn >::argument_types

#define Q_FIRST_ARGUMENT_OF( Fn ) \
	typename ::q::function_traits< Fn >::argument_types::first_type

#define Q_FIRST_ARGUMENT_IS_TUPLE( Fn ) \
	::q::is_tuple< \
		typename ::std::decay< \
			typename ::q::function_traits< \
				Fn \
			>::argument_types::first_type \
		>::type \
	>::value

#define Q_ARGUMENTS_ARE( Fn, ... ) \
	::q::is_argument_same_t< \
		Q_ARGUMENTS_OF( Fn ), \
		::q::arguments< __VA_ARGS__ > \
	>

#define Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( Fn, ... ) \
	::q::is_argument_same_or_convertible_t< \
		::q::arguments< __VA_ARGS__ >, \
		Q_ARGUMENTS_OF( Fn ) \
	>

#define Q_ARGUMENTS_ARE_CONVERTIBLE_FROM_INCL_VOID( Fn, ... ) \
	::q::is_argument_same_or_convertible_incl_void_t< \
		::q::arguments< __VA_ARGS__ >, \
		Q_ARGUMENTS_OF( Fn ) \
	>

#define Q_MEMBERCLASS_OF( Fn ) \
	typename ::q::function_traits< Fn >::memberclass_type

#define Q_IS_MEMBERFUNCTION( Fn ) \
	::q::bool_type_t< !std::is_void< Q_MEMBERCLASS_OF( Fn ) >::value >

#define Q_IS_FUNCTION( Fn ) \
	::q::function_traits< Fn >::valid

#define Q_IS_CONST_OF( Fn ) \
	::q::function_traits< Fn >::is_const::value;

namespace q {

namespace detail {

struct valid_t
{
	typedef std::true_type valid;
};

struct invalid_t
{
	typedef std::false_type valid;
};

template< typename Signature >
struct fn_match
: public invalid_t
{ };

template< typename R, typename... A >
struct fn_match< R( & )( A... ) >
: public fn_match< R( A... ) >
{ };

template< typename R, typename... A >
struct fn_match< R( * )( A... ) >
: public fn_match< R( A... ) >
{ };

template< typename R, typename... A >
struct fn_match< R( A... ) >
: public valid_t
{
	typedef R result_type;
	typedef arguments< A... > argument_types;
	typedef R( signature )( A... );
	typedef R( *signature_ptr )( A... );
	typedef void memberclass_type;
	typedef void member_signature_ptr;
	typedef std::false_type is_const;
};

template< typename R, typename C, typename... A >
struct fn_match< R( C::* )( A... ) >
: public fn_match< R( C::* )( A... ) const >
{
	typedef std::false_type is_const;
};

template< typename R, typename C, typename... A >
struct fn_match< R( C::* )( A... ) const >
: public valid_t
{
	typedef R result_type;
	typedef arguments< A... > argument_types;
	typedef R( signature )( A... );
	typedef R( *signature_ptr )( A... );
	typedef C memberclass_type;
	typedef R( C::*member_signature_ptr )( A... );
	typedef std::true_type is_const;
};


template< typename Fn >
fn_match< Fn > fn_type( Fn&& );

template< typename Fn, typename Enabled = void >
struct has_call_operator
: public std::false_type
{ };

template< typename Fn >
struct has_call_operator<
	Fn,
	typename std::enable_if<
		std::is_same<
			decltype( q::detail::fn_type(
				&std::decay< Fn >::type::operator( ) ) ),
			decltype( q::detail::fn_type(
				&std::decay< Fn >::type::operator( ) ) )
		>::value
	>::type
>
: public std::true_type
{ };

template<
	typename Fn,
	bool Valid = fn_match< Fn >::valid::value,
	bool CallableByOperator = has_call_operator< Fn >::value
>
struct identity
{
	typedef typename std::remove_reference< Fn >::type decayed_type;
	typedef fn_match< decayed_type > match;
	typedef std::false_type using_call_operator;
};

template< typename Fn >
struct identity< Fn, false, true >
{
	typedef typename std::remove_reference< Fn >::type decayed_type;
	typedef decltype( fn_type( &decayed_type::operator( ) ) ) match;
	typedef std::true_type using_call_operator;
};

struct invalid_match
: public invalid_t
{
	typedef void result_type;
	typedef arguments< > argument_types;
	typedef void memberclass_type;
	typedef void signature;
	typedef void signature_ptr;
	typedef void member_signature_ptr;
	typedef void is_const;
};

// Fn is not any kind of function
template< typename Fn >
struct identity< Fn, false, false >
{
	typedef typename std::decay< Fn >::type decayed_type;
	typedef invalid_match match;
	typedef std::false_type using_call_operator;
};

template< typename Fn >
struct function_traits
{
	typedef Fn type;
	typedef identity< type > ident;
	typedef typename ident::match match;
	typedef typename ident::using_call_operator using_call_operator;
};

} // namespace detail

template< typename Fn >
struct function_traits
: public detail::function_traits< Fn >
{
	using typename detail::function_traits< Fn >::type;

	// MSVC 2015 gets an ICE on this 'using'. Similar to GCC below on
	// 'match'... This seems hard for compilers to get right.
	//using typename detail::function_traits< Fn >::using_call_operator;
	typedef typename detail::function_traits<
		Fn
	>::using_call_operator                       using_call_operator;

	typedef bool_type<
		!using_call_operator::value
	>                                            plain;

	// TODO: GCC Seems to not allow this using statement, which is why the
	// typedef is necessary.
	//using typename detail::function_traits< Fn >::match;
	typedef typename detail::function_traits< Fn >::match match;

	/**
	 * std::true_type or std::false_type depending on whether the type is
	 * understood to be a valid function or function-like type or not
	 */
	typedef typename match::valid                valid;

	/**
	 * The result type of the function. Same as @c std::result_type
	 */
	typedef typename match::result_type          result_type;

	/**
	 * The result type of the function capsulated in a q::arguments. This will
	 * treat a void return type (which really isn't a type) as an empty type
	 * list of the q::arguments. Hence this type becomes q::arguments<>.
	 */
	typedef typename std::conditional<
		std::is_same< result_type, void >::value,
		arguments< >,
		::q::tuple_arguments_t< result_type >
	>::type                                      result_argument_type;

	/**
	 * A @c q::arguments type for the argument types
	 */
	typedef typename match::argument_types       argument_types;

	/**
	 * The type of the class for which this function is a member, for member
	 * functions, or void for non-member function types.
	 */
	typedef typename match::memberclass_type     memberclass_type;

	/**
	 * The plain function type on the form result_type( argument types... )
	 */
	typedef typename match::signature            signature;

	/**
	 * Function pointer type on the form result_type( * )( argument types... )
	 */
	typedef typename match::signature_ptr        signature_ptr;

	/**
	 * Member function pointer type on the form:
	 * result_type( memberclass_type::* )( argument types... )
	 */
	typedef typename match::member_signature_ptr member_signature_ptr;

	/**
	 * std::function-wrapped type
	 */
	typedef std::function< signature >           function_type;

	/**
	 * The arity of the function, i.e. the number of arguments it expects
	 */
	typedef typename argument_types::size        arity;

	/**
	 * If this is a member function, is_const is std::true_type if it is a
	 * const member function. Is std::false_type otherwise.
	 * This also affects capturing lambas, where is_const will be
	 * std::false_type if the lambda is `mutable` otherwise std::true_type.
	 */
	typedef typename match::is_const             is_const;
};

template< typename Fn >
using is_function_t = typename Q_IS_FUNCTION( Fn );

template< typename Fn >
using is_plain_function_t = typename ::q::function_traits< Fn >::plain;

template< typename Fn >
using signature_ptr_of_t = typename ::q::function_traits< Fn >::signature_ptr;

template< typename Fn >
using member_signature_ptr_of_t =
	typename ::q::function_traits< Fn >::member_signature_ptr;

template< typename Fn >
using std_function_of_t = typename ::q::function_traits< Fn >::function_type;

template< typename Fn >
using is_memberfunction_t = typename Q_IS_MEMBERFUNCTION( Fn );

template< typename Fn >
using arity_of_t = typename ::q::function_traits< Fn >::arity;

template< typename Fn >
using is_const_of_t = typename ::q::function_traits< Fn >::is_const;

// Mutable are function objects with non-const call operators
template< typename Fn >
using is_mutable_of_t = bool_type<
	!::q::function_traits< Fn >::is_const::value
	and
	::q::function_traits< Fn >::using_call_operator::value
>;

template< typename Fn >
using result_of_t = Q_RESULT_OF( Fn );

template< typename Fn >
using result_of_as_argument_t = Q_RESULT_OF_AS_ARGUMENT_TYPE( Fn );

template< typename Fn >
using result_of_as_tuple_t = Q_RESULT_OF_AS_TUPLE_TYPE( Fn );

template< typename Fn >
using arguments_of_t = Q_ARGUMENTS_OF( Fn );

template< typename Fn >
using first_argument_of_t = Q_FIRST_ARGUMENT_OF( Fn );

template< typename Fn >
using first_argument_is_tuple_t = ::q::is_tuple<
		typename ::std::decay<
			typename ::q::function_traits<
				Fn
			>::argument_types::first_type
		>::type
	>;

template< typename Fn >
using memberclass_of_t = Q_MEMBERCLASS_OF( Fn );

template< typename Fn, typename... Args >
using arguments_of_are_t =
	::q::is_argument_same_t<
		arguments_of_t< Fn >,
		::q::arguments< Args... >
	>;

template< typename Fn, typename... Args >
using arguments_of_are_convertible_from_t =
	::q::is_argument_same_or_convertible_t<
		::q::arguments< Args... >,
		arguments_of_t< Fn >
	>;

template< typename Fn, typename... Args >
using arguments_of_are_convertible_from_incl_void_t =
	::q::is_argument_same_or_convertible_incl_void_t<
		::q::arguments< Args... >,
		arguments_of_t< Fn >
	>;

template<
	typename Fn,
	template< typename > class T,
	bool _HasAnyArgument = ( arity_of_t< Fn >::value > 0 )
>
struct first_argument_is
: std::false_type
{ };

template<
	typename Fn,
	template< typename > class T
>
struct first_argument_is< Fn, T, true >
: T< first_argument_of_t< Fn > >
{ };

template< typename Fn, bool HasAnyArgument = ( arity_of_t< Fn >::value > 0 ) >
struct first_argument_is_tuple
: std::false_type
{ };

template< typename Fn >
struct first_argument_is_tuple< Fn, true >
: bool_type_t< first_argument_is_tuple_t< Fn >::value >
{ };

namespace detail {

template< typename Fn, bool IsTuple, typename Arguments >
struct _tuple_arguments_of_are_convertible_from_incl_void_
{
	typedef std::false_type type;
};

template< typename Fn, typename Arguments >
struct _tuple_arguments_of_are_convertible_from_incl_void_< Fn, true, Arguments >
{
	typedef bool_type<
		Arguments
		::template is_convertible_to_incl_void<
			typename std::decay<
				tuple_arguments_t<
					first_argument_of_t< Fn >
				>
			>::type
		>::value
	> type;
};

template< typename Fn, std::size_t Arity, typename Arguments >
struct _tuple_arguments_of_are_convertible_from_incl_void
{
	typedef std::false_type type;
};

template< typename Fn, typename Arguments >
struct _tuple_arguments_of_are_convertible_from_incl_void< Fn, 1, Arguments >
{
	typedef typename _tuple_arguments_of_are_convertible_from_incl_void_<
		Fn,
		is_tuple<
			typename std::decay< first_argument_of_t< Fn > >::type
		>::value,
		Arguments
	>::type type;
};

} // namespace detail

template< typename Fn, typename Arguments >
using tuple_arguments_of_are_convertible_from_incl_void_t =
	typename detail::_tuple_arguments_of_are_convertible_from_incl_void<
		Fn,
		arguments_of_t< Fn >::size::value,
		Arguments
	>::type;

template<
	typename Fn,
	typename Arguments = arguments< result_of_t< Fn > >,
	bool Empty = Arguments::empty_or_voidish::value
>
struct result_of_is_voidish_or_eventually_voidish
{
	typedef std::true_type type;
};

template< typename Fn, typename Arguments >
struct result_of_is_voidish_or_eventually_voidish< Fn, Arguments, false >
{
	typedef typename Arguments::first_type::argument_types::empty_or_voidish type;
};

template< typename Fn >
using result_of_is_voidish_or_eventually_voidish_t =
	typename result_of_is_voidish_or_eventually_voidish< Fn >::type;


#ifdef LIBQ_WITH_CPP14

template< typename Fn >
constexpr bool is_function_v = is_function_t< Fn >::value;

template< typename Fn >
constexpr bool is_plain_function_v = is_plain_function_t< Fn >::value;

template< typename Fn >
constexpr bool is_memberfunction_v = is_memberfunction_t< Fn >::value;

template< typename Fn >
constexpr std::size_t arity_of_v = arity_of_t< Fn >::value;

template< typename Fn >
constexpr bool is_const_of_v = is_const_of_t< Fn >::value;

template< typename Fn >
constexpr bool is_mutable_of_v = is_mutable_of_t< Fn >::value;

template< typename Fn, typename... Args >
constexpr bool arguments_of_are_v = arguments_of_are_t< Fn, Args... >::value;

template< typename Fn, typename... Args >
constexpr bool arguments_of_are_convertible_from_v =
	arguments_of_are_convertible_from_t< Fn, Args... >::value;

template< typename Fn, typename... Args >
constexpr bool arguments_of_are_convertible_from_incl_void_v =
	arguments_of_are_convertible_from_incl_void_t< Fn, Args... >::value;

#endif // C++14


/**
 * decay_function ensures that the function provided is either a function
 * pointer (possibly a member function pointer!), or a std::function<...>.
 *
 * In complex situations, compilers can be fooled to coerce a function with an
 * implicit signature (lambdas, or "anonymous classes") into a function with a
 * different signature. This shouldn't be compilable but actually is.
 *
 * By forwarding the function through decay_function, it is guaranteed to
 * either be a function pointer or a std::function, in which case the compiler
 * cannot make coercion mistakes.
 */
template< typename Fn >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	is_convertible_to_t< Fn, signature_ptr_of_t< Fn > >::value,
	signature_ptr_of_t< Fn >
>::type
decay_function( Fn&& fn )
{
	return static_cast< signature_ptr_of_t< Fn > >(
		std::forward< Fn >( fn ) );
}

template< typename Fn >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	is_convertible_to_t< Fn, member_signature_ptr_of_t< Fn > >::value,
	signature_ptr_of_t< Fn >
>::type
decay_function( Fn&& fn )
{
	return static_cast< member_signature_ptr_of_t< Fn > >(
		std::forward< Fn >( fn ) );
}

template< typename Fn >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	!is_convertible_to_t< Fn, signature_ptr_of_t< Fn > >::value
	and
	!is_convertible_to_t< Fn, member_signature_ptr_of_t< Fn > >::value
	and
	is_convertible_to_t< Fn, std_function_of_t< Fn > >::value,
	std_function_of_t< Fn >
>::type
decay_function( Fn&& fn )
{
	return static_cast< std_function_of_t< Fn > >(
		std::forward< Fn >( fn ) );
}

template< typename Fn >
struct decayed_function
{
	typedef decltype( decay_function( std::declval< Fn >( ) ) ) type;
};

template< typename Fn >
struct decayed_function< Fn& >
: decayed_function< Fn >
{ };

template< typename Fn >
struct decayed_function< const Fn >
: decayed_function< Fn >
{ };

template< typename Fn >
using decayed_function_t = typename decayed_function< Fn >::type;

namespace detail {

template< typename Fn1, typename Fn2 >
struct function_cmp
{
	typedef bool_type_t<
		arguments_of_t< Fn1 >
		::template equals< arguments_of_t< Fn2 > >::value
		and
		result_of_as_argument_t< Fn1 >
		::template equals< result_of_as_argument_t< Fn2 > >::value
	> equal;

	typedef bool_type_t<
		arguments_of_t< Fn1 >
		::template map< functional_type< std::decay >::of_t >
		::template equals<
			typename arguments_of_t< Fn2 >
			::template map< functional_type< std::decay >::of_t >
		>::value
		and
		result_of_as_argument_t< Fn1 >
		::template map< functional_type< std::decay >::of_t >
		::template equals<
			typename result_of_as_argument_t< Fn2 >
			::template map< functional_type< std::decay >::of_t >
		>::value
	> same_type;
};

} // namespace detail

template< typename Fn1, typename Fn2 >
using function_equal_t = typename detail::function_cmp< Fn1, Fn2 >::equal;

template< typename Fn1, typename Fn2 >
using function_same_type_t =
	typename detail::function_cmp< Fn1, Fn2 >::same_type;

#ifdef LIBQ_WITH_CPP14

template< typename Fn1, typename Fn2 >
constexpr bool function_equal_v = function_equal_t< Fn1, Fn2 >::value;

template< typename Fn1, typename Fn2 >
constexpr bool function_same_type_v = function_same_type_t< Fn1, Fn2 >::value;

#endif // LIBQ_WITH_CPP14


template< typename Fn, typename... Args >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( Fn, Args... )::value
	and
	(
		!Q_IS_MEMBERFUNCTION( Fn )::value
		or
		Q_FUNCTIONTRAITS( Fn )::using_call_operator::value
	),
	result_of_t< Fn >
>::type
call_with_args( Fn&& fn, Args&&... args )
#ifndef LIBQ_WITH_BROKEN_NOEXCEPT
noexcept( noexcept( fn( std::forward< Args >( args )... ) ) )
#endif
{
	return fn( std::forward< Args >( args )... );
}

template< typename Fn >
typename std::enable_if<
	is_function_t< Fn >::value
	and
	Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( Fn, void_t )::value
	and
	(
		!Q_IS_MEMBERFUNCTION( Fn )::value
		or
		Q_FUNCTIONTRAITS( Fn )::using_call_operator::value
	),
	result_of_t< Fn >
>::type
call_with_args( Fn&& fn )
#ifndef LIBQ_WITH_BROKEN_NOEXCEPT
noexcept( noexcept( fn( void_t( ) ) ) )
#endif
{
	return fn( void_t( ) );
}

template< typename Fn, typename Class, typename... Args >
typename std::enable_if<
	Q_IS_MEMBERFUNCTION( Fn )::value &&
	(
		std::is_same<
			Q_MEMBERCLASS_OF( Fn )*,
			typename std::decay< Class >::type
		>::value ||
		::q::is_base_of<
			Q_MEMBERCLASS_OF( Fn )*,
			typename std::decay< Class >::type
		>::value
	) &&
	is_pointer_like< typename std::decay< Class >::type >::value,
	result_of_t< Fn >
>::type
call_with_args( Fn&& fn, Class&& obj, Args&&... args )
#ifndef LIBQ_WITH_BROKEN_NOEXCEPT
noexcept( noexcept( ( obj->*fn )( std::forward< Args >( args )... ) ) )
#endif
{
	return ( obj->*fn )( std::forward< Args >( args )... );
}

template< typename Fn, typename Class, typename... Args >
typename std::enable_if<
	Q_IS_MEMBERFUNCTION( Fn )::value &&
	(
		std::is_same<
			Q_MEMBERCLASS_OF( Fn ),
			typename std::decay< Class >::type
		>::value ||
		::q::is_base_of<
			Q_MEMBERCLASS_OF( Fn ),
			typename std::decay< Class >::type
		>::value
	) &&
	!is_pointer_like< typename std::decay< Class >::type >::value,
	result_of_t< Fn >
>::type
call_with_args( Fn&& fn, Class&& obj, Args&&... args )
#ifndef LIBQ_WITH_BROKEN_NOEXCEPT
noexcept( noexcept( ( obj.*fn )( std::forward< Args >( args )... ) ) )
#endif
{
	return ( obj.*fn )( std::forward< Args >( args )... );
}

namespace detail {

template< typename Fn, class Tuple, std::size_t... Indexes >
result_of_t< Fn >
call_with_args_by_tuple( Fn&& fn, Tuple&& tuple, q::index_tuple< Indexes... > )
#ifndef LIBQ_WITH_BROKEN_NOEXCEPT
noexcept( noexcept(
	Q_RESULT_OF( Fn )(
		call_with_args(
			std::forward< Fn >( fn ),
			std::forward<
				typename std::tuple_element<
					Indexes,
					typename std::remove_reference<
						Tuple
					>::type
				>::type
			>( std::get< Indexes >( tuple ) )...
		)
	)
) )
#endif
{
	return call_with_args(
		std::forward< Fn >( fn ),
		std::forward<
			typename std::tuple_element<
				Indexes,
				typename std::remove_reference< Tuple >::type
			>::type
		>( std::get< Indexes >( tuple ) )... );
}

template< typename Tuple >
using index_tuple = typename q::make_index_tuple<
		q::tuple_arguments_t< Tuple >::size::value
	>::type;

} // namespace detail

template< typename Fn, class Tuple >
typename std::enable_if<
	!std::is_same<
		typename std::decay< Tuple >::type,
		std::tuple< >
	>::value,
	result_of_t< Fn >
>::type
call_with_args_by_tuple( Fn&& fn, Tuple&& tuple )
#ifndef LIBQ_WITH_BROKEN_NOEXCEPT
noexcept( noexcept(
	detail::call_with_args_by_tuple(
		std::forward< Fn >( fn ),
		std::forward< Tuple >( tuple ),
		detail::index_tuple< Tuple >( ) )
) )
#endif
{
	return detail::call_with_args_by_tuple(
		std::forward< Fn >( fn ),
		std::forward< Tuple >( tuple ),
		detail::index_tuple< Tuple >( )
	);
}

template< typename Fn >
typename std::enable_if<
	!Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( Fn, void_t )::value,
	result_of_t< Fn >
>::type
call_with_args_by_tuple( Fn&& fn, const std::tuple< >& )
#ifndef LIBQ_WITH_BROKEN_NOEXCEPT
noexcept( noexcept( fn( ) ) )
#endif
{
	return fn( );
}

template< typename Fn >
typename std::enable_if<
	Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( Fn, void_t )::value,
	Q_RESULT_OF( Fn )
>::type
call_with_args_by_tuple( Fn&& fn, const std::tuple< >& )
#ifndef LIBQ_WITH_BROKEN_NOEXCEPT
noexcept( noexcept( fn( void_t( ) ) ) )
#endif
{
	return fn( void_t( ) );
}

template< typename Fn, typename InnerFn, typename... Args >
typename std::enable_if<
	Q_IS_FUNCTION( Fn )::value
	and
	Q_IS_FUNCTION( InnerFn )::value
	and
	Q_RESULT_OF_AS_ARGUMENT( InnerFn )::size::value == 0
	and
	Q_ARITY_OF( Fn ) == 0
	and
	Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( InnerFn, Args... )::value,
	Q_RESULT_OF( Fn )
>::type
call_with_args_by_fun( Fn&& fn, InnerFn&& inner_fn, Args&&... args )
#ifndef LIBQ_WITH_BROKEN_NOEXCEPT
noexcept(
	noexcept( inner_fn( std::forward< Args >( args )... ) )
	and
	noexcept( Q_RESULT_OF( Fn )( fn( ) ) )
)
#endif
{
	inner_fn( std::forward< Args >( args )... );
	return fn( );
}

template< typename Fn, typename InnerFn, typename... Args >
typename std::enable_if<
	Q_IS_FUNCTION( Fn )::value
	and
	Q_IS_FUNCTION( InnerFn )::value
	and
	( Q_RESULT_OF_AS_ARGUMENT( InnerFn )::size::value > 0 )
	and
	::q::is_argument_same_or_convertible_t<
		Q_RESULT_OF_AS_ARGUMENT_TYPE( InnerFn ),
		Q_ARGUMENTS_OF( Fn )
	>::value
	and
	Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( InnerFn, Args... )::value,
	result_of_t< Fn >
>::type
call_with_args_by_fun( Fn&& fn, InnerFn&& inner_fn, Args&&... args )
#ifndef LIBQ_WITH_BROKEN_NOEXCEPT
noexcept( noexcept(
	Q_RESULT_OF( Fn )( fn( inner_fn( std::forward< Args >( args )... ) ) )
) )
#endif
{
	return fn( inner_fn( std::forward< Args >( args )... ) );
}

template< typename Class, typename Fn, typename... Args >
typename std::enable_if<
	Q_RESULT_OF_AS_ARGUMENT( Fn )::size::value == 0
	and
	std::is_default_constructible< Class >::value,
	Class
>::type
construct_with_function_call( Fn&& fn, Args&&... args )
#ifndef LIBQ_WITH_BROKEN_NOEXCEPT
noexcept(
	noexcept( fn( std::forward< Args >( args )... ) )
	and
	noexcept( Class( Class( ) ) )
)
#endif
{
	fn( std::forward< Args >( args )... );
	return Class( );
}

template< typename Class, typename Fn, typename... Args >
typename std::enable_if<
	( Q_RESULT_OF_AS_ARGUMENT( Fn )::size::value > 0 )
	and
	merge<
		std::is_constructible,
		q::arguments< Class >,
		Q_RESULT_OF_AS_ARGUMENT_TYPE( Fn )
	>::type::value,
	Class
>::type
construct_with_function_call( Fn&& fn, Args&&... args )
#ifndef LIBQ_WITH_BROKEN_NOEXCEPT
noexcept( noexcept( Class( Class( fn( std::forward< Args >( args )... ) ) ) ) )
#endif
{
	return Class( fn( std::forward< Args >( args )... ) );
}

/**
 * Automatically deduces the types of the @a args and tries to call @a fn with
 * these arguments, or if @a args is a tuple or @c q::arguments instance,
 * tries to unpack it so that it matches the arguments to @a fn.
 *
 * In other words, a function void( int, double ) can be invoked with @a args
 * being either ( int, double ) or an instance of std::tuple< int, double > or
 * q::arguments< int, double >.
 */
template< typename Fn, typename... Args >
result_of_t< Fn >
call( Fn&& fn, Args&&... args )
{
	// TODO: Implement
}

} // namespace q

#endif // LIBQ_FUNCTIONAL_HPP
