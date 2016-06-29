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

#ifndef LIBQ_TEST_STRINGIFY_HPP
#define LIBQ_TEST_STRINGIFY_HPP

namespace q { namespace test { namespace internal {

static const std::string new_line = "\n";

struct can_stream_value_impl
{
	template< typename T >
	static std::true_type
	test_value( typename std::decay< decltype(
		std::declval< std::ostream& >( ) << std::declval< T >( )
	) >::type* );

	template< typename T >
	static std::false_type
	test_value( ... );

	template< typename T >
	struct test
	: decltype( test_value< T >( nullptr ) )
	{ };
};

template< typename T >
struct can_stream_value
: can_stream_value_impl::template test< T >
{ };

template< typename T >
static typename std::enable_if<
	!q::is_tuple< typename std::decay< T >::type >::value
	and
	can_stream_value< T >::value,
	std::string
>::type stringify_value( const T& t, bool = false )
{
	std::stringstream ss;
	ss << t;
	return ss.str( );
}

const std::string unstringifyable = "{non-printable value}";

template< typename T >
static typename std::enable_if<
	!q::is_tuple< typename std::decay< T >::type >::value
	and
	!can_stream_value< T >::value,
	const std::string&
>::type stringify_value( T&& t, bool = false )
{
	return unstringifyable;
}

namespace detail {

template< std::size_t Size, typename... T >
struct tuple_stringifyer
{
	template< std::size_t Index >
	static typename std::enable_if<
		Index < Size - 1,
		std::string
	>::type
	stringify_tuple( const std::tuple< T... >& t )
	{
		std::stringstream ss;
		ss
			<< stringify_value( std::get< Index >( t ), true )
			<< ", "
			<< stringify_tuple< Index + 1 >( t );
		return ss.str( );
	}

	template< std::size_t Index >
	static typename std::enable_if<
		Index == Size - 1,
		std::string
	>::type
	stringify_tuple( const std::tuple< T... >& t )
	{
		std::stringstream ss;
		ss << stringify_value( std::get< Index >( t ), true ) << " ";
		return ss.str( );
	}
};

template< >
struct tuple_stringifyer< 0 >
{
	template< std::size_t >
	static std::string stringify_tuple( const std::tuple< >& t )
	{
		return "";
	}
};

} // namespace detail

template< typename... T >
struct tuple_stringifyer
: detail::tuple_stringifyer< sizeof...( T ), T... >
{ };

template< typename... T >
struct tuple_stringifyer< std::tuple< T... > >
: detail::tuple_stringifyer< sizeof...( T ), T... >
{ };

template< typename T >
static typename std::enable_if<
	q::is_tuple< typename std::decay< T >::type >::value,
	std::string
>::type stringify_value(
	const T& t, bool explicit_tuple = false
)
{
	typedef typename std::decay< T >::type tuple_type;

	std::stringstream ss;

	if ( explicit_tuple || std::tuple_size< tuple_type >::value > 1 )
		ss << "tuple< ";

	ss << tuple_stringifyer< tuple_type >
		::template stringify_tuple< 0 >( t );

	if ( explicit_tuple || std::tuple_size< tuple_type >::value > 1 )
		ss << ">";

	return ss.str( );
}

} } } // namespace internal, namespace test, namespace q

#endif // LIBQ_TEST_STRINGIFY_HPP
