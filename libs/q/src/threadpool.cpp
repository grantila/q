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
	{ }

	typedef std::shared_ptr< thread< > >         thread_type;
	typedef expect< void >                       result_type;
	typedef promise< std::tuple< result_type > > promise_type;

	queue_ptr                         queue_;
	std::string                       name_;
	mutex                             mutex_;
	std::size_t                       num_threads_;
	std::vector< thread_type >        threads_;
	std::condition_variable           cond_;
	bool                              started_;
	bool                              running_;
	bool                              stop_asap_;
	task_fetcher_task                 task_fetcher_;
};

threadpool::threadpool( const std::string& name,
                        const queue_ptr& queue,
                        std::size_t threads )
: event_dispatcher( queue )
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

void threadpool::notify( )
{
	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );
	}
	pimpl_->cond_.notify_one( );
}

void threadpool::set_task_fetcher( task_fetcher_task&& fetcher )
{
	pimpl_->task_fetcher_ = std::move( fetcher );

	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );
	}
	pimpl_->cond_.notify_all( );
}

void threadpool::start( )
{
	auto _this = shared_from_this( );

	auto terminated = std::make_shared< std::atomic< size_t > >( 0 );
	auto num_threads = pimpl_->num_threads_;

	for ( std::size_t index = 0; index < num_threads; ++index )
	{
		auto thread_name = make_thread_name(
			pimpl_->name_, index + 1, pimpl_->num_threads_ );

		auto fn = [ _this, index ]( ) -> void
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
				if ( pimpl_->stop_asap_ )
					break;

				task _task = pimpl_->task_fetcher_
					? std::move( pimpl_->task_fetcher_( ) )
					: task( );

				if ( !pimpl_->running_ && !_task )
					break;

				if ( _task )
				{
					Q_AUTO_UNIQUE_UNLOCK( lock );

					invoker( std::move( _task ) );
				}

				if ( pimpl_->running_ && !_task )
					pimpl_->cond_.wait( lock );
			}
			while ( true );

			_this->mark_completion( );
		};

		auto t = run( std::move( thread_name ),
		              pimpl_->queue_,
		              std::move( fn ) );

		t->terminate( )
		.finally( [ _this, terminated, num_threads ]( )
		{
			auto prev = terminated->fetch_add(
				1, std::memory_order_seq_cst );

			if ( prev == num_threads - 1 )
			{
				// Last thread to terminate, signal complete
				// thread pool termination
				_this->termination_done( );
			}
		} );
		// TODO: .fail()-handler

		pimpl_->threads_.push_back( std::move( t ) );
	}
}

void threadpool::mark_completion( )
{
	pimpl_->cond_.notify_all( );
}

void threadpool::do_terminate( termination method )
{

	// 1. Wait for all jobs to terminate
	// 2. Disallow new jobs
	// ...

	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );

		pimpl_->running_ = false;

		if ( method != q::termination::linger )
			pimpl_->stop_asap_ = true;
	}

	pimpl_->cond_.notify_all( );
}

q::expect< > threadpool::await_termination( )
{
	// TODO: Forward potential errors from individual threads.
	//       They shouldn't exist (only due to internal bugs) but still.
	for ( auto& t : pimpl_->threads_ )
	{
		t->await_termination( );
	}

	return ::q::fulfill< void >( );
}

} // namespace q
