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

#ifndef LIBQ_PROMISE_PROMISE_IMPL_REST_HPP
#define LIBQ_PROMISE_PROMISE_IMPL_REST_HPP

namespace q { namespace detail {

template< bool Shared, typename... Args >
template< typename Logger, typename Queue >
inline typename std::enable_if<
	is_same_type< Logger, log_chain_generator >::value
	and
	Q_IS_SETDEFAULT_SAME( queue_ptr, Queue ),
	typename generic_promise<
		Shared, std::tuple< Args... >
	>::promise_this_type
>::type
generic_promise< Shared, std::tuple< Args... > >::
then( Logger&& logger, Queue&& queue )
{
	Q_MAKE_MOVABLE( logger );

	auto fn = [ Q_MOVABLE_FORWARD( logger ) ]( tuple_type args ) -> void
	{
		call_with_args_by_tuple( Q_MOVABLE_CONSUME( logger ), args );
	};
	std::packaged_task< void( void ) > task; // TODO: REDO

	auto future = task.get_future( );

	// queue->push( fn );

	return promise_this_type( std::move( future ) );
}

template< bool Shared, typename... Args >
typename generic_promise< Shared, std::tuple< Args... > >::promise_this_type
generic_promise< Shared, std::tuple< Args... > >::
delay( timer::duration_type duration, queue_options options )
{
	auto queue = options.move< queue_ptr >( queue_ );
	auto next_queue = options.move< defaultable< queue_ptr > >(
		set_default( queue ) ).value;

	auto deferred = ::q::make_shared< detail::defer< Args... > >(
		next_queue );

	auto state = state_;

	auto perform = [ deferred, state ]( ) mutable
	{
		auto value = state->consume( );
		deferred->set_expect( std::move( value ) );
	};

	auto timed_task = [ perform, queue, duration ]( ) mutable
	{
		auto wait_until = timer::point_type::clock::now( ) + duration;

		queue->push( perform, wait_until );
	};

	state_->signal( )->push( std::move( timed_task ), queue );

	return deferred->template get_suitable_promise< promise_this_type >( );
}

template< bool Shared, typename... Args >
q::promise< std::tuple<
	typename generic_promise<
		Shared, std::tuple< Args... >
	>::tuple_expect_type
> >
generic_promise< Shared, std::tuple< Args... > >::
reflect_tuple( )
{
	auto deferred = ::q::make_shared< detail::defer<
		::q::expect< std::tuple< Args... > >
	> >( get_queue( ) );
	auto state = state_;

	auto perform = [ deferred, state ]( ) mutable
	{
		auto value = state->consume( );

		deferred->set_value( std::move( value ) );
	};

	state_->signal( )->push( std::move( perform ), get_queue( ) );

	return deferred->get_promise( );
}

template< bool Shared, typename... Args >
template< bool Simplified >
typename std::enable_if<
	Simplified,
	::q::promise< std::tuple<
		typename generic_promise<
			Shared, std::tuple< Args... >
		>::short_expect_type
	> >
>::type
generic_promise< Shared, std::tuple< Args... > >::
reflect( )
{
	auto deferred = ::q::make_shared< detail::defer<
		::q::expect< Args... >
	> >( get_queue( ) );
	auto state = state_;

	auto perform = [ deferred, state ]( ) mutable
	{
		auto value = state->consume( );

		deferred->set_inner_expect( std::move( value ) );
	};

	state_->signal( )->push( std::move( perform ), get_queue( ) );

	return deferred->get_promise( );
}

template< bool Shared, typename... Args >
template< typename... U, typename _V >
typename std::enable_if<
	std::is_void< _V >::value
	and
	generic_promise< Shared, std::tuple< Args... > >
		::argument_types::empty::value,
	promise< std::tuple< U... > >
>::type
generic_promise< Shared, std::tuple< Args... > >::
forward( U&&... values )
{
	auto deferred = ::q::make_shared< detail::defer< U... > >(
		get_queue( ) );
	auto state = state_;

	auto tup = std::make_tuple( std::forward< U >( values )... );
	Q_MAKE_MOVABLE( tup );

	auto perform = [ Q_MOVABLE_MOVE( tup ), deferred, state ]( ) mutable
	{
		auto value = state->consume( );
		if ( value.has_exception( ) )
			// Redirect exception
			deferred->set_exception( value.exception( ) );
		else
			deferred->set_value( Q_MOVABLE_CONSUME( tup ) );
	};

	state_->signal( )->push( std::move( perform ), get_queue( ) );

	return deferred->get_promise( );
}

} } // namespace detail, namespace q

namespace q {

template< typename T >
::q::promise< std::tuple< > > promise< T >::
strip( )
{
	return this->then( [ ]( const typename base_type::tuple_type& ){ } );
}

template< typename T >
::q::shared_promise< std::tuple< > > shared_promise< T >::
strip( )
{
	return this->then( [ ]( const typename base_type::tuple_type& ){ } )
	.share( );
}

} // namespace q


#endif // LIBQ_PROMISE_PROMISE_IMPL_REST_HPP
