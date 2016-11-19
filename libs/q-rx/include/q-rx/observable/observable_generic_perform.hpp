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

#ifndef LIBQ_RX_OBSERVABLE_GENERIC_PERFORM_HPP
#define LIBQ_RX_OBSERVABLE_GENERIC_PERFORM_HPP

namespace q { namespace rx {

/*

TODO: Make default concurrency == event dispatcher parallelism, expose through queue

Pre-conditions:
	ConcurrencyCanRead <
		Multi-threaded backend AND "Ongoing tasks" < Concurrency
		else
		true


Read (consume) if:
	Can push forward (front backlog not full) AND ConcurrencyCanRead

Run:
	If concurrency == 1:
		If queue is mapped to a single-threaded event dispatcher:
			If fn is sync:
		(a)		Push fn+data to the queue
			Else fn returns Promise:
		(b)		Push 1 fn+data, await it before pushing next
		Else (multi-threaded event dispatcher):
		(c)	Push 1 fn+data and await its completion
	Else concurrency > 1
		If queue is mapped to a single-threaded event dispatcher:
			If fn is sync:
		(d)		Just push, order will be guaranteed
			Else fn returns Promise:
		(e)		Push fn+data to the queue, await it, ensure
				maximum <concurrency> ongoing promises before
				pushing the next.
				Ensure order will be guaranteed
		Else (multi-threaded event dispatcher):
		(f)	Push n fn+data to the queue, await the corresponding
			promise result (if any) before sending more.
			n is the concurrency.
			Ensure

Push:
	At any time, since reading checks if pushing is possible
	Although,
	Handle errors (upstream/downstream)


Transposed:
	If Parallelism == 1 AND Sync:
		Consume
		Run, eventually maybe Push
		If/When should_send(), iterate
	Consume (resolve, reject):


*/

namespace detail {

template< typename T, typename = std::false_type >
struct generic_perform_has_on_close
{
	typedef void no;
};
template< typename T >
struct generic_perform_has_on_close<
	T,
	std::enable_if_t< is_function_v< typename T::on_close > >
>
{
	typedef void yes;
};

template<
	typename T,
	typename Writable,
	typename generic_perform_has_on_close< T >::yes* = nullptr
>
auto
generic_perform_on_close( Writable&& writable, std::shared_ptr< T > performer )
{
	return [ performer{ std::move( performer ) } ]( ) mutable
	{
		return performer->on_close( );
	};
}

template<
	typename T,
	typename Writable,
	typename generic_perform_has_on_close< T >::no* = nullptr
>
auto
generic_perform_on_close( Writable&& writable, std::shared_ptr< T > performer )
{
	return [ writable{ std::forward< Writable >( writable ) } ]( ) mutable
	{
		writable.close( );
	};
}

template< typename T, typename = std::false_type >
struct generic_perform_has_on_error
{
	typedef void no;
};
template< typename T >
struct generic_perform_has_on_error<
	T,
	std::enable_if_t< is_function_v< typename T::on_error > >
>
{
	typedef void yes;
};

template<
	typename T,
	typename Writable,
	typename generic_perform_has_on_error< T >::yes* = nullptr
>
auto
generic_perform_on_error( Writable&& writable, std::shared_ptr< T > performer )
{
	return [ performer{ std::move( performer ) } ]( std::exception_ptr e )
	mutable
	{
		return performer->on_error( e );
	};
}

template<
	typename T,
	typename Writable,
	typename generic_perform_has_on_error< T >::no* = nullptr
>
auto
generic_perform_on_error( Writable&& writable, std::shared_ptr< T > performer )
{
	return [ writable{ std::forward< Writable >( writable ) } ]
	( std::exception_ptr e ) mutable
	{
		writable.close( e );
	};
}

} // namespace detail


struct generic_perform_sync;
struct generic_perform_async;


template< typename T >
template< typename Operator >
observable< typename Operator::element_type >
observable< T >::
generic_perform( std::shared_ptr< Operator > performer, queue_ptr next_queue )
{
	typedef typename Operator::element_type element_type;
	using traits = typename detail::observable_types< element_type >;
	typedef typename traits::channel_type out_channel_type;

	// TODO: Implement concurrency
	// TODO: Make better default backlog

	out_channel_type _channel(
		std::move( next_queue ), Q_RX_DEFAULT_BACKLOG );

	auto writable = _channel.get_writable( );

	performer->setup( writable );

	auto on_data = [ writable, performer ]( void_safe_type t ) mutable
	{
		return performer->on_data( std::move( t ) );
	};
	auto on_close = detail::generic_perform_on_close( writable, performer );
	auto on_error = detail::generic_perform_on_error( writable, performer );

	consume( std::move( on_data ) )
	.then( std::move( on_close ) )
	.fail( std::move( on_error ) );

	return observable< element_type >( _channel.get_readable( ) );
}

} } // namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_GENERIC_PERFORM_HPP
