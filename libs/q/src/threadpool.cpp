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

#include <q/threadpool.hpp>
#include <q/mutex.hpp>

#include <thread>
#include <queue>
#include <condition_variable>
#include <sstream>
#include <set>

#include <unistd.h>

namespace q {

namespace {

static std::string make_thread_name( const std::string& base,
                                     std::size_t num,
                                     std::size_t total )
{
	std::stringstream ret;
	ret << base;
	if ( total > 1 )
		ret << " (#" << num << "/" << total << ")";
	return ret.str( );
}

} // anonymous namespace

static std::unique_ptr< task > make_unique_task( task&& t )
{
	return std::unique_ptr< task >( new task( std::move( t ) ) );
}

struct pool_task_item
{
	std::unique_ptr< task >            task_;
//	std::unique_ptr< task_time_point > wait_until;
};

static bool operator<( const pool_task_item& left, const pool_task_item& right )
{
	/*
	if ( left.wait_until && right.wait_until )
		return *left.wait_until < *right.wait_until;
	return !left.wait_until;
	 */
}

struct threadpool::pimpl
{
	pimpl( const queue_ptr& queue,
	       const std::string& name,
	       std::size_t threads )
	: queue_( queue )
	, name_( name )
	, mutex_( Q_HERE, "[" + name + "] mutex" )
	, num_threads_( threads )
	, started_( false )
	, running_( false )
	, stop_asap_( false )
	, allow_more_jobs_( true )
	{ }

	typedef std::shared_ptr< thread< > >         thread_type;
	typedef expect< void >                       result_type;
	typedef promise< std::tuple< result_type > > promise_type;

	queue_ptr                         queue_;
	std::string                       name_;
	mutex                             mutex_;
	std::size_t                       num_threads_;
	std::vector< thread_type >        threads_;
	std::queue< pool_task_item >      tasks_;
	std::set< pool_task_item >        delayed_tasks_;
	std::condition_variable           cond_;
	bool                              started_;
	bool                              running_;
	bool                              stop_asap_;
	bool                              allow_more_jobs_;
};

threadpool::threadpool( const std::string& name,
                        const queue_ptr& queue,
                        std::size_t threads )
: async_event_dispatcher( queue )
, pimpl_( new pimpl( queue, name, threads ) )
{
	pimpl_->running_ = true;
	pimpl_->started_ = true;
	pimpl_->threads_.reserve( threads );
}

threadpool::~threadpool( )
{
	;
}

std::shared_ptr< threadpool >
threadpool::construct( const std::string& name,
                       const queue_ptr& queue,
                       std::size_t threads )
{
	auto tp = ::q::make_shared_using_constructor< threadpool >(
		name, queue, threads );
	tp->start( );
	return tp;
}

void threadpool::add_task( task task )
{
	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );

		if ( pimpl_->started_ && !pimpl_->allow_more_jobs_ )
		{
			if ( !pimpl_->running_ )
			{
				// Silenty ignore jobs
			}
			else
			{
				// We aren't shutting down, but still not allow
				// more jobs?
				// TODO: throw something, or re-design
			}
		}
		else
		{
			pimpl_->tasks_.push(
				{ make_unique_task( std::move( task ) ) } );
		}
	}

	pimpl_->cond_.notify_one( );
}
/*
void threadpool::add_task( task task, task_time_point wait_until )
{
	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );

		if ( pimpl_->started_ && !pimpl_->allow_more_jobs_ )
		{
			// TODO: Do stuff, as above
		}
		else
		{
			std::unique_ptr< task_time_point > time(
				new task_time_point(
					std::move( wait_until ) ) );

			pool_task_item pti = {
				make_unique_task( std::move( task ) ),
				std::move( time )
			};

			pimpl_->delayed_tasks_.insert( std::move( pti ) );
		}
	}

	pimpl_->cond_.notify_one( );
}
*/
void threadpool::start( )
{
	auto _this = shared_from_this( );

	std::vector< pimpl::promise_type > thread_completions;

	auto predicate = [ _this ]( )
	{
		auto& pimpl_ = _this->pimpl_;

		return !pimpl_->running_ || !pimpl_->tasks_.empty( );
	};

	auto fetch_next_task = [ _this ]( )
	-> pool_task_item
	{
		auto& pimpl_ = _this->pimpl_;
		pool_task_item ret;

		if ( !pimpl_->delayed_tasks_.empty( ) )
		{
			// TODO: Make this a bit more efficient, to check how
			// many tasks are now runnable (might be many). If
			// above a certain threshold (like 4 or something),
			// move the tasks from the delayed_tasks_ map into the
			// start of the tasks_ queue so that
/*
			auto now = task_clock::now( );

			auto first = pimpl_->delayed_tasks_.begin( );
			if ( *first->wait_until < now )
			{
				ret.task_ = make_unique_task(
					std::move( *first->task_ ) );

				pimpl_->delayed_tasks_.erase( first );
			}
			else if ( pimpl_->tasks_.empty( ) )
			{
				ret.wait_until.reset(
					new task_time_point(
						*first->wait_until ) );
			}
*/
		}

		if ( !ret.task_ && !pimpl_->tasks_.empty( ) )
		{
			ret.task_ = make_unique_task(
				std::move( *pimpl_->tasks_.front( ).task_ ) );
			pimpl_->tasks_.pop( );
		}

		return ret;
	};

	for ( std::size_t num = 1; num <= pimpl_->num_threads_; ++num )
	{
		auto thread_name = make_thread_name(
			pimpl_->name_, num, pimpl_->num_threads_ );

		auto fn = [ _this, predicate, fetch_next_task ]( )
		{
			auto& pimpl_ = _this->pimpl_;

			auto lock = Q_UNIQUE_LOCK( pimpl_->mutex_ );

			auto invoker = [ ]( task&& elem )
			{
				try
				{
					elem( );
				}
				catch ( ... )
				{
					LIBQ_UNCAUGHT_EXCEPTION(
						std::current_exception( ) );
				}
			};

			do
			{
				auto item = fetch_next_task( );

				if ( item.task_ )
				{
					Q_AUTO_UNIQUE_UNLOCK( lock );

					invoker( std::move( *item.task_ ) );
				}
/*
				if ( item.wait_until )
				{
					pimpl_->cond_.wait_until(
						lock,
						*item.wait_until,
						predicate );
				}
				else
				{
*/
					pimpl_->cond_.wait( lock, predicate );
/*
				}
*/
			}
			while ( pimpl_->running_ );

			_this->mark_completion( );

			pimpl_->cond_.wait( lock );
		};

		auto t = run( std::move( thread_name ),
		              pimpl_->queue_,
		              std::move( fn ) );

		auto promise = t->terminate( );

		thread_completions.push_back( std::move( promise ) );

		pimpl_->threads_.push_back( std::move( t ) );
	}
	/*
	 all( std::move( thread_completions ) )
	 .then( [ this ]( std::vector< pimpl::result_type >&& results )
	 {
	 this->termination_done( );
	 } );
	 */
}

void threadpool::mark_completion( )
{
	pimpl_->cond_.notify_all( );
}

void threadpool::do_terminate( )
{

	// 1. Wait for all jobs to terminate
	// 2. Disallow new jobs
	// ...

	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );

		pimpl_->running_ = false;
		pimpl_->allow_more_jobs_ = false;
	}

	pimpl_->cond_.notify_all( );
}

q::expect< > threadpool::await_termination( )
{
	;
}

} // namespace q
