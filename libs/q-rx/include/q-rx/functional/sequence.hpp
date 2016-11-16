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

#ifndef LIBQ_RX_FUNCTIONAL_SEQUENCE_HPP
#define LIBQ_RX_FUNCTIONAL_SEQUENCE_HPP

#include <q-rx/observable.hpp>

#include <q/functional.hpp>

namespace q { namespace rx { namespace f {

template< typename T >
typename std::enable_if<
	is_observable_v< T >,
	typename T::value_type
>::type
accumulate( T&& sequence )
{
	typedef typename T::value_type element_type;
	return sequence.reduce( [ ]( element_type&& prev, element_type&& cur )
	{
		return prev + cur;
	}, element_type( ) );
}

template< typename T >
typename std::enable_if<
	is_observable_v< T >,
	T
>::type
adjacent_difference( T&& sequence )
{
	typedef typename T::value_type element_type;
	return sequence.scan( [ ]( element_type&& prev, element_type&& cur )
	{
		return cur - prev;
	}, element_type( ) );
}

} } } // namespace f, namespace rx, namespace q

#endif // LIBQ_RX_FUNCTIONAL_SEQUENCE_HPP
