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

#ifndef LIBQ_EXECUTION_CONTEXT_HPP
#define LIBQ_EXECUTION_CONTEXT_HPP

#include <q/types.hpp>
#include <q/type_traits.hpp>
#include <q/event_dispatcher.hpp>
#include <q/blocking_dispatcher.hpp>
#include <q/scheduler.hpp>

#include <memory>

namespace q {

class execution_context
{
public:
	~execution_context( );

	queue_ptr queue( ) const;

	scheduler_ptr scheduler( ) const;

protected:
	execution_context( event_dispatcher_ptr ed, const scheduler_ptr& s );

private:
	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

template< typename Dispatcher >
class specific_execution_context
: public execution_context
{
public:
	std::shared_ptr< Dispatcher > dispatcher( )
	{
		return ed_;
	}

protected:
	specific_execution_context(
		std::shared_ptr< Dispatcher > ed,
		const scheduler_ptr& s )
	: execution_context( ed, s )
	, ed_( ed )
	{ }

	std::shared_ptr< Dispatcher > ed_;
};

namespace detail {

template< typename EventDispatcher >
typename std::enable_if<
	std::is_same<
		typename std::decay< EventDispatcher >::type::autostart_tag,
		event_dispatcher_yes_tag
	>::value,
	std::true_type
>::type
should_auto_start( EventDispatcher&& );

std::false_type should_auto_start( const basic_event_dispatcher& );

template< typename EventDispatcher >
struct event_dispatcher_type_traits
{
	typedef decltype(
		should_auto_start( std::declval< EventDispatcher >( ) )
	) autostart;
};

template< typename EventDispatcher >
typename std::enable_if<
	std::is_base_of<
		q::enable_queue_from_this,
		typename std::decay< EventDispatcher >::type
	>::value
>::type
set_event_dispatcher_queue(
	EventDispatcher&& event_dispatcher, const queue_ptr& queue
)
{
	event_dispatcher.set_queue( queue );
}

template< typename EventDispatcher >
typename std::enable_if<
	!std::is_base_of<
		q::enable_queue_from_this,
		typename std::decay< EventDispatcher >::type
	>::value
>::type
set_event_dispatcher_queue(
	EventDispatcher&& event_dispatcher, const queue_ptr& queue
)
{ }

} // namespace detail

template<
	typename EventDispatcher,
	typename Scheduler = ::q::direct_scheduler,
	typename... Args
>
specific_execution_context_ptr< EventDispatcher >
make_execution_context( Args&&... args )
{
	auto ed = q::make_shared< EventDispatcher >(
		std::forward< Args >( args )... );
	auto s = q::make_shared< Scheduler >( ed );

	auto ec = q::make_shared<
		specific_execution_context< EventDispatcher >
	>( ed, s );

	set_event_dispatcher_queue( *ed, ec->queue( ) );

	if (
		detail::event_dispatcher_type_traits< EventDispatcher >
			::autostart::value
	)
		ed->start( );

	return ec;
}

template<
	typename EventDispatcher,
	typename Scheduler = ::q::direct_scheduler,
	typename... Args
>
std::tuple< std::shared_ptr< EventDispatcher >, queue_ptr >
make_event_dispatcher_and_queue( Args&&... args )
{
	auto ec = make_execution_context< EventDispatcher, Scheduler >(
		std::forward< Args >( args )... );

	return std::make_pair( ec->dispatcher( ), ec->queue( ) );
}

} // namespace q

#endif // LIBQ_EXECUTION_CONTEXT_HPP
