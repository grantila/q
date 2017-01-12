/*
 * Copyright 2017 Gustaf Räntilä
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

#ifndef LIBQ_RX_FUNCTIONAL_FUNCTION_HPP
#define LIBQ_RX_FUNCTIONAL_FUNCTION_HPP

#include <q-rx/observable.hpp>

#include <q/functional.hpp>

namespace q { namespace rx { namespace f {

namespace {

template<
	typename Only,
	std::enable_if_t< q::is_tuple_v< std::decay_t< Only > > >* = nullptr
>
auto tuple_flat_cat( Only&& only )
{
	return only;
}

template<
	typename Only,
	std::enable_if_t< !q::is_tuple_v< std::decay_t< Only > > >* = nullptr
>
auto tuple_flat_cat( Only&& only )
{
	return std::forward_as_tuple< Only >( only );
}

template<
	typename First,
	typename Second,
	typename... Rest,
	std::enable_if_t< q::is_tuple_v< std::decay_t< Second > > >* = nullptr
>
auto tuple_flat_cat( First&& first, Second&& second, Rest&&... rest )
{
	return tuple_flat_cat(
		std::tuple_cat(
			std::forward< First >( first ),
			std::forward< Second >( second )
		),
		std::forward< Rest >( rest )...
	);
}

template<
	typename First,
	typename Second,
	typename... Rest,
	std::enable_if_t< !q::is_tuple_v< std::decay_t< Second > > >* = nullptr
>
auto tuple_flat_cat( First&& first, Second&& second, Rest&&... rest )
{
	return tuple_flat_cat(
		std::tuple_cat(
			std::forward< First >( first ),
			std::make_tuple( std::forward< Second >( second ) )
		),
		std::forward< Rest >( rest )...
	);
}

template< bool IsVoid, bool IsPromise >
struct append_result_helper
{
	template< typename Fn >
	static auto wrap( Fn&& fn )
	{
		return [ fn{ std::forward< Fn >( fn ) } ]( auto&&... args )
		{
			auto ret = fn( args... );
			return tuple_flat_cat(
				std::make_tuple( std::move( args )... ), ret );
		};
	}
};

template< >
struct append_result_helper< true, false > // void-specialization
{
	template< typename Fn >
	static auto wrap( Fn&& fn )
	{
		return [ fn{ std::forward< Fn >( fn ) } ]( auto&&... args )
		{
			fn( args... );
			return std::make_tuple( std::move( args )... );
		};
	}
};

template< >
struct append_result_helper< false, true > // promise-specialization
{
	template< typename Fn >
	static auto wrap( Fn&& fn )
	{
		using return_type = typename q::result_of_t< Fn >::tuple_type;

		return [ fn{ std::forward< Fn >( fn ) } ]( auto&&... args )
		{
			return fn( args... )
			.then( [ args... ]( return_type&& ret )
			{
				return tuple_flat_cat(
					std::make_tuple( args... ),
					std::move( ret )
				);
			} );
		};
	}
};

} // anonymous namespace

template<
	typename Fn,
	bool IsVoid = q::is_voidish_v< result_of_t< Fn > >,
	bool IsPromise = q::is_promise_v< result_of_t< Fn > >
>
auto append_result( Fn&& fn )
{
	return append_result_helper< IsVoid, IsPromise >::wrap(
		std::forward< Fn >( fn ) );
}

} } } // namespace f, namespace rx, namespace q

#endif // LIBQ_RX_FUNCTIONAL_FUNCTION_HPP
