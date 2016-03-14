
#include <q/blocking_dispatcher.hpp>
#include <q/mutex.hpp>

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
	, should_wait_( true )
	, started_( false )
	, running_( false )
	, stop_asap_( false )
	{ }

	std::string name_;
	mutex running_mutex_;
	mutex mutex_;
	std::condition_variable cond_;
	bool should_wait_;
	bool started_;
	bool running_;
	bool stop_asap_;
	task_fetcher_task task_fetcher_;
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
	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );
		pimpl_->should_wait_ = false;
	}
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

			_task( );
		}

		if ( !_task ) //&& pimpl_->should_wait_ )
		{
			pimpl_->cond_.wait( lock );
			pimpl_->should_wait_ = true;
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
