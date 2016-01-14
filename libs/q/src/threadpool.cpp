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
	return q::make_unique< task >( std::move( t ) );
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
	std::queue< task >                tasks_;
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
				// TODO: Do better warning
				std::cerr
					<< "[libq] threadpool \""
					<< pimpl_->name_
					<< "\" got task while shutting down - "
					<< "throwing away task"
					<< std::endl;
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
			pimpl_->tasks_.push( std::move( task ) );
		}
	}

	pimpl_->cond_.notify_one( );
}

void threadpool::start( )
{
	auto _this = shared_from_this( );

	auto predicate = [ _this ]( )
	{
		auto& pimpl_ = _this->pimpl_;

		return !pimpl_->running_ || !pimpl_->tasks_.empty( );
	};

	auto fetch_next_task = [ _this ]( )
	-> std::unique_ptr< task >
	{
		auto& pimpl_ = _this->pimpl_;
		std::unique_ptr< task > ret;

		if ( !pimpl_->tasks_.empty( ) )
		{
			ret = q::make_unique_task(
				std::move( pimpl_->tasks_.front( ) ) );
			pimpl_->tasks_.pop( );
		}

		return ret;
	};

	auto terminated = std::make_shared< std::atomic< size_t > >( 0 );
	auto num_threads = pimpl_->num_threads_;

	for ( std::size_t num = 1; num <= num_threads; ++num )
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
				if ( !pimpl_->running_ )
				{
					if ( !item )
						// No more tasks and we've
						// stopped running.
						break;
				}

				if ( item )
				{
					Q_AUTO_UNIQUE_UNLOCK( lock );

					invoker( std::move( *item ) );
				}

				if ( pimpl_->running_ )
					pimpl_->cond_.wait( lock, predicate );
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
			// TODO: Actually implement support for continuing to
			//       allow more tasks. This currently doesn't work.
			pimpl_->allow_more_jobs_ = false;
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
