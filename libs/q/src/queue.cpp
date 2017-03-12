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

#include <q/queue.hpp>
#include <q/mutex.hpp>
#include <q/memory.hpp>
#include <q/exception.hpp>

#include <queue>

namespace q {

// TODO: Consider using a semaphore instead, and then preferably a non-locking
// queue altogether. The only thing necessary is that two push-calls from the
// same thread must follow order.
struct queue::pimpl
{
	pimpl( priority_t priority )
	: priority_( priority )
	, mutex_( Q_HERE, "queue mutex" )
	, parallelism_( 1 )
	{ }

	const priority_t priority_;
	mutex mutex_;
	queue::notify_type notify_;
	std::size_t parallelism_;
	std::queue< task > queue_;
	std::queue< timer_task > timer_task_queue_;
};

queue_ptr queue::construct( priority_t priority )
{
	return make_shared_using_constructor< queue >( priority );
}

queue::queue( priority_t priority )
: pimpl_( new pimpl( priority ) )
{
}

queue::~queue( )
{
}

void queue::push( task&& task )
{
	notify_type notifyer;

	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_, Q_HERE, "queue::push" );

		pimpl_->queue_.push( std::move( task ) );

		notifyer = pimpl_->notify_;
	}

	if ( notifyer )
		notifyer( );
}

void queue::push( task&& task, timer::point_type wait_until )
{
	notify_type notifyer;

	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_, Q_HERE, "queue::push(2)" );

		timer_task tt( std::move( task ), std::move( wait_until ) );
		pimpl_->timer_task_queue_.push( std::move( tt ) );

		notifyer = pimpl_->notify_;
	}

	if ( notifyer )
		notifyer( );
}

priority_t queue::priority( ) const
{
	return pimpl_->priority_;
}

void queue::set_consumer( queue::notify_type fn, std::size_t parallelism )
{
	Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_, Q_HERE, "queue::set_consumer" );

	pimpl_->notify_ = fn;
	pimpl_->parallelism_ = parallelism;
}

bool queue::empty( )
{
	Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_, Q_HERE, "queue::empty" );

	return pimpl_->queue_.empty( ) && pimpl_->timer_task_queue_.empty( );
}

timer_task queue::pop( )
{
	Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_, Q_HERE, "queue::pop" );

	if ( !pimpl_->timer_task_queue_.empty( ) )
	{
		timer_task task = std::move( pimpl_->timer_task_queue_.front( ) );

		pimpl_->timer_task_queue_.pop( );

		return task;
	}

	if ( pimpl_->queue_.empty( ) )
		return timer_task( );

	timer_task task = std::move( pimpl_->queue_.front( ) );

	pimpl_->queue_.pop( );

	return task;
}

std::size_t queue::parallelism( ) const
{
	return pimpl_->parallelism_;
}

} // namespace q
