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

#ifndef LIBQ_MEMORY_HPP
#define LIBQ_MEMORY_HPP

#include <memory>
#include <q/functional.hpp>

namespace q {

namespace detail {

template< typename T, typename... Args >
struct has_custom_static_construct
{
	template< typename C >
	static std::true_type test_normal( decltype( &C::construct ) );

	template< typename C, typename... FnArgs >
	static std::true_type
	test_templated( decltype( &C::template construct< FnArgs... > ) );

	template< typename C >
	static std::false_type
	test_normal( ... );

	template< typename C, typename... FnArgs >
	static std::false_type
	test_templated( ... );

	typedef decltype( test_normal< T >( nullptr ) ) normal;
	typedef decltype( test_templated< T, Args... >( nullptr ) ) templated;
};

}

/**
 * Constructor wrapper for classes with enable_shared_from_this i.e. they have
 * protected constructors to enforce construction elseway (preferably to
 * shared pointers).
 * This helper wrapper makes such construction not only possible but effective
 * as std::make_shared will work, and thereby make a memory efficient
 * reference counter for the object.
 *
 * You will not want to use this class directly, but rather to use the type
 * deducing helper function @c q::make_shared().
 */
template< typename T >
class shared_constructor
: public T
{
public:
	template< typename... Args >
	shared_constructor( Args&&... args )
	: T( std::forward< Args >( args )... )
	{ }
};


template< typename T, typename... Args >
std::shared_ptr< T >
make_shared_using_constructor( Args&&... args )
{
	typedef shared_constructor< T > sub_type;
	return std::make_shared< sub_type >( std::forward< Args >( args )... );
}

/**
 * Helper function used to construct a shared_ptr of any class which has a
 * public or protected constructor with arbitrary arguments.
 *
 * This function is almost identical to std::make_shared() although it can
 * instanciate classes with protected constructors, while std::make_shared()
 * requires public constructors, which may not be suitable if the class is
 * supposed to be shared (e.g. subclassing std::enable_shared_from_this<>).
 *
 * A class which has a static function 'construct(...)' will be constructed
 * using that function, rather than using std::make_shared. If this pattern is
 * followed, classes which are supposed to be shared will all be creatable
 * using this function.
 * NOTE: There must only be one function called 'construct(...)', if there are
 * multiple functions with that name, q will not be able to recognize it, and
 * will choose to use the constructor instead.
 */
template< typename T, typename... Args >
typename std::enable_if<
	!detail::has_custom_static_construct< T, Args... >::normal::value &&
	!detail::has_custom_static_construct< T, Args... >::templated::value,
	std::shared_ptr< T >
>::type
make_shared( Args&&... args )
{
	return make_shared_using_constructor< T >(
		std::forward< Args >( args )... );
}

template< typename T, typename... Args >
typename std::enable_if<
	detail::has_custom_static_construct< T, Args... >::normal::value,
	std::shared_ptr< T >
>::type
make_shared( Args&&... args )
{
	return T::construct( std::forward< Args >( args )... );
}

template< typename T, typename... Args >
typename std::enable_if<
	detail::has_custom_static_construct< T, Args... >::templated::value,
	std::shared_ptr< T >
>::type
make_shared( Args&&... args )
{
	return T::template construct< Args... >(
		std::forward< Args >( args )...
	);
}

} // namespace q

#endif // LIBQ_MEMORY_HPP
