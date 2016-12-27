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

#ifndef LIBQ_RX_OBSERVABLE_TRANSFORMERS_MAP_HPP
#define LIBQ_RX_OBSERVABLE_TRANSFORMERS_MAP_HPP

namespace q { namespace rx {

namespace {

template< typename from_type, typename fn_type, typename _element_type >
struct map_context
: public std::enable_shared_from_this<
	map_context< from_type, fn_type, _element_type >
>
{
	// Generic perform traits
	typedef _element_type element_type;
//	typedef generic_perform_sync execution;

	typedef map_context< from_type, fn_type, element_type > this_type;
	using traits = typename detail::observable_types< element_type >;
	typedef typename traits::channel_type out_channel_type;
	typedef typename traits::writable_type out_writable_type;
	typedef objectify< from_type > void_safe_type;

	std::shared_ptr< this_type > self_;
	queue_ptr queue_;
	fn_type fn_;
	out_writable_type writable_;

	template< typename Fn >
	map_context(
		queue_ptr&& queue,
		q::concurrency concurrency,
		Fn&& fn
	)
	: queue_( std::move( queue ) )
	, fn_( std::forward< Fn >( fn ) )
	{ }

	void setup( out_writable_type writable )
	{
		writable_ = std::move( writable );
	}

	promise< > on_data( from_type&& in )
	{
		writable_.send( fn_( std::move( in ) ) );

		return backpressure::await_writable( queue_, writable_ );
	}

	// TODO: Implement concurrency
	// TODO: Implement back pressure handling
};

} // anonymous namespace

/**
 * ( In ) -> Out
 */
template< typename T >
template< typename Fn >
typename std::enable_if<
	q::arguments_of_are_convertible_from_incl_void_v< Fn, T >
	and
	!q::is_promise< q::result_of_t< Fn > >::value,
	observable< q::result_of_t< Fn > >
>::type
observable< T >::
map( Fn&& fn, base_options options )
{
	typedef q::result_of_t< Fn > element_type;

	auto next_queue = options.get< q::defaultable< q::queue_ptr > >(
		q::set_default( readable_->get_queue( ) ) ).value;
	auto queue = options.get< q::queue_ptr >( next_queue );
	auto concurrency = options.get< q::concurrency >( );

	// TODO: We can inline the map-function with the consumption
	const bool inline_map_function = get_queue( ) == queue;

	typedef map_context<
		T,
		std::decay_t< Fn >,
		element_type
	> context_type;

	auto ctx = std::make_shared< context_type >(
		std::move( queue ),
		concurrency,
		std::forward< Fn >( fn )
	);

	return generic_perform( ctx, std::move( next_queue ) );
}

/**
 * ( In ) -> promise< Out >
 */
template< typename T >
template< typename Fn >
typename std::enable_if<
	q::arguments_of_are_convertible_from_incl_void_v< Fn, T >
	and
	q::is_promise< q::result_of_t< Fn > >::value,
	typename detail::tuple_to_observable<
		typename ::q::result_of_t< Fn >::tuple_type
	>::type
>::type
observable< T >::
map( Fn&& fn, base_options options )
{
	// TODO: Implement
	return *this;
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_TRANSFORMERS_MAP_HPP
