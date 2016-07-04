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

#ifndef LIBQ_RX_OBSERVABLE_CORE_HPP
#define LIBQ_RX_OBSERVABLE_CORE_HPP

namespace q { namespace rx {

typedef q::options<
	queue_ptr,
	q::defaultable< q::queue_ptr >,
	q::concurrency
> base_options;

namespace detail {

template< typename T, std::size_t Size = std::tuple_size< T >::value >
struct tuple_to_observable
{
	typedef observable< T > type;
};

template< typename T >
struct tuple_to_observable< T, 0 >
{
	typedef observable< void > type;
};

template< typename T >
struct tuple_to_observable< T, 1 >
{
	typedef observable< typename std::tuple_element< 0, T >::type > type;
};

} // namespace detail

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_CORE_HPP
