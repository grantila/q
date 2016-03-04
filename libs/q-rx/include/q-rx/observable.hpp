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

#ifndef LIBQ_RX_OBSERVABLE_HPP
#define LIBQ_RX_OBSERVABLE_HPP

#include <q/functional.hpp>
#include <q/promise.hpp>
#include <q/channel.hpp>

namespace q { namespace rx {

template< typename T >
class observable
{
public:
	observable( q::readable_ptr< T >&& readable )
	: readable_( std::move( readable ) )
	{ }

	/**
	 * ( In ) -> promise< Out >
	 */
	template< typename Fn >
	typename std::enable_if<
		q::is_promise< Q_RESULT_OF( Fn ) >::value,
		observable<
			typename std::conditional<
				std::tuple_size<
					::q::result_of< Fn >::tuple_type
				>::value == 0,
				void,
				typename std::conditional<
					std::tuple_size<
						::q::result_of< Fn >::tuple_type
					>::value == 1,
					typename std::tuple_element<
						0,
						::q::result_of< Fn >::tuple_type
					>::type,
					::q::result_of< Fn >::tuple_type
				>::type
			>::type
		>
	>::type
	map( Fn&& fn );

	// TODO: Implemement properly, e.g. with promises
	T onNext( );
	std::exception_ptr onError( );
	void onComplete( );

private:
	q::readable_ptr< T > readable_;
};

} // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_HPP
