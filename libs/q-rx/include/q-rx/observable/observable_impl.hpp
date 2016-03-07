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

#ifndef LIBQ_RX_OBSERVABLE_OBSERVABLE_IMPL_HPP
#define LIBQ_RX_OBSERVABLE_OBSERVABLE_IMPL_HPP

namespace q { namespace rx {

template< typename T >
struct is_observable
: std::false_type
{ };

template< typename T >
struct is_observable< observable< T > >
: std::true_type
{ };

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_OBSERVABLE_IMPL_HPP
