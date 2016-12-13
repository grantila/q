
#include <q/blocking_dispatcher.hpp>
#include <q/mutex.hpp>
#include <q/time_set.hpp>

#include <queue>

namespace q {

struct blocking_dispatcher::pimpl
{
	pimpl( )
	: pimpl( "" )
	{ }

	pimpl( const std::string& name )
	: name_( name )
	, mutex_( Q_HERE, "[" + name + "] mutex" )
	, started_( false )
	, running_( false )
	, stop_asap_( false )
	{ }

	std::string name_;
	mutex running_mutex_;
	mutex mutex_;
	std::condition_variable cond_;
	bool started_;
	bool running_;
	bool stop_asap_;
	task_fetcher_task task_fetcher_;
	time_set< task > timer_tasks_;
};

blocking_dispatcher::blocking_dispatcher(
	const std::string& name )
: sync_event_dispatcher< q::arguments< termination > >( )
, pimpl_( new pimpl( name ) )
{ }

blocking_dispatcher::~blocking_dispatcher( )
{
	;
}

void blocking_dispatcher::notify( )
{
	pimpl_->cond_.notify_one( );
}

void blocking_dispatcher::set_task_fetcher( task_fetcher_task&& fetcher )
{
	pimpl_->task_fetcher_ = std::move( fetcher );
}

void blocking_dispatcher::start( )
{
	Q_AUTO_UNIQUE_LOCK( pimpl_->running_mutex_ );
	auto lock = Q_UNIQUE_LOCK( pimpl_->mutex_ );

	pimpl_->running_ = true;
	pimpl_->started_ = true;

	auto duration_max = timer::duration_type::max( );

	do
	{
		if ( pimpl_->stop_asap_ )
			break;

		if ( pimpl_->timer_tasks_
			.exists_before_or_at( )
		)
		{
			auto task = pimpl_->timer_tasks_.pop( );

			if ( task )
			{
				Q_AUTO_UNIQUE_UNLOCK( lock );

				task( );

				continue;
			}
		}

		timer_task _task = pimpl_->task_fetcher_
			? pimpl_->task_fetcher_( )
			: timer_task( );

		if ( !pimpl_->running_ && !_task )
			break;

		if ( _task.is_timed( ) )
		{
			// We just add the timed task and re-iterate.
			// This handling needs a complete overhaul. TODO
			pimpl_->timer_tasks_.push(
				std::move( _task.wait_until ),
				std::move( _task.task ) );

			continue;
		}
		else if ( _task )
		{
			Q_AUTO_UNIQUE_UNLOCK( lock );

			_task.task( );
		}

		if ( pimpl_->running_ && !_task )
		{
			if ( !pimpl_->timer_tasks_.empty( ) )
			{
				auto next = pimpl_->timer_tasks_.next_time( );

				if ( next != duration_max )
				{
					pimpl_->cond_.wait_for(
						lock, next );
					continue;
				}
			}
			pimpl_->cond_.wait( lock );
		}
	}
	while ( true );
}

void blocking_dispatcher::do_terminate( termination method )
{
	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );

		pimpl_->running_ = false;

		switch ( method )
		{
			case termination::linger:
				break;
			case termination::annihilate:
				pimpl_->stop_asap_ = true;
		}
	}

	pimpl_->cond_.notify_one( );
}

q::expect< > blocking_dispatcher::await_termination( )
{
	Q_AUTO_UNIQUE_LOCK( pimpl_->running_mutex_ );
	return q::fulfill< void >( );
}

} // namespace q
