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

#ifndef LIBQ_TYPE_TRAITS_CONVERTIBLE_HPP
#define LIBQ_TYPE_TRAITS_CONVERTIBLE_HPP

namespace q {

template< typename From, typename To >
struct is_convertible_to
{
	typedef logic_or<
		is_same_type< From, To >, std::is_convertible< From, To >
	> type;
};

template< typename From, typename To >
using is_convertible_to_t = typename is_convertible_to< From, To >::type;

} // namespace q

#endif // LIBQ_TYPE_TRAITS_CONVERTIBLE_HPP
