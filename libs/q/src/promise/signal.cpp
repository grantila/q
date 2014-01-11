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

struct item
{
	task task_;
	queue_ptr queue_;
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

void promise_signal::done( ) noexcept // TODO: analyze noexcept here
{
	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );

		pimpl_->done_ = true;
	}

	for ( auto item : pimpl_->items_ )
		item.queue_->push( std::move( item.task_ ) );

	pimpl_->items_.clear( );
}

void promise_signal::push( task&& task, const queue_ptr& queue )
{
	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );

		if ( !pimpl_->done_ )
		{
			pimpl_->items_.push_back( { std::move( task ), queue } );

			return;
		}
	}

	queue->push( std::move( task ) );
}

} } // namespace detail, namespace queue
