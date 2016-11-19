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

#include <q/promise/signal.hpp>

#include <q/mutex.hpp>
#include <q/queue.hpp>

namespace q { namespace detail {

namespace {

/**
 * items are tasks bound to a certain queue.
 *
 * They can also be synchronous tasks which run directly when the promise is
 * resolved. These must be tiny and fast and they must be 'noexcept'. This is
 * used for scheduling custom async tasks for event loops, e.g. timers.
 */
struct item
{
	task task_;
	queue_ptr queue_;
	bool synchronous_;
};

} // anonymous namespace

struct promise_signal::pimpl
{
	mutex mutex_;
	bool done_;
	std::vector< item > items_;
};

promise_signal::promise_signal( )
: pimpl_( new pimpl )
{
	pimpl_->done_ = false;
}

promise_signal::~promise_signal( )
{ }

// TODO: Analyze noexcept here, when it comes to pushing to a queue which might
//       be closed or similar.
void promise_signal::done( ) noexcept
{
	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );

		pimpl_->done_ = true;
	}

	for ( auto& item : pimpl_->items_ )
	{
		if ( item.synchronous_ )
			item.task_( );
		else
			item.queue_->push( std::move( item.task_ ) );
	}

	pimpl_->items_.clear( );
}

void promise_signal::push( task&& task, const queue_ptr& queue ) noexcept
{
	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );

		if ( !pimpl_->done_ )
		{
			pimpl_->items_.push_back(
				{ std::move( task ), queue, false } );

			return;
		}
	}

	queue->push( std::move( task ) );
}

void promise_signal::push_synchronous( task&& task ) noexcept
{
	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );

		if ( !pimpl_->done_ )
		{
			pimpl_->items_.push_back(
				{ std::move( task ), nullptr, true } );

			return;
		}
	}

	task( );
}

} } // namespace detail, namespace queue
