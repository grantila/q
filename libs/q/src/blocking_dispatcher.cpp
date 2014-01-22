
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
	, started_( false )
	, running_( false )
	, stop_asap_( false )
	, allow_more_jobs_( true )
	{ }

	std::string name_;
	mutex mutex_;
	std::queue< task > tasks_;
	std::condition_variable cond_;
	bool started_;
	bool running_;
	bool stop_asap_;
	bool allow_more_jobs_;
};

blocking_dispatcher::blocking_dispatcher( const std::string& name )
: pimpl_( new pimpl( name ) )
{ }

blocking_dispatcher::~blocking_dispatcher( )
{
	;
}

void blocking_dispatcher::add_task( task task )
{
	{
		Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );

		if ( !pimpl_->started_ )
		{
			pimpl_->tasks_.push( std::move( task ) );
		}
		else if ( !pimpl_->allow_more_jobs_ )
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
			pimpl_->tasks_.push( std::move( task ) );
		}
	}

	pimpl_->cond_.notify_one( );
}

void blocking_dispatcher::start( )
{
	auto lock = Q_UNIQUE_LOCK( pimpl_->mutex_ );

	pimpl_->running_ = true;
	pimpl_->started_ = true;

	auto predicate = [ this ]( )
	{
		return !pimpl_->running_ || !pimpl_->tasks_.empty( );
	};

	do
	{
		if ( !pimpl_->tasks_.empty( ) )
		{
			auto elem = std::move( pimpl_->tasks_.front( ) );
			pimpl_->tasks_.pop( );

			Q_AUTO_UNIQUE_UNLOCK( lock );

			// Invoke task
			elem( );
		}

		pimpl_->cond_.wait( lock, predicate );
	}
	while ( pimpl_->running_ );

	{
		Q_AUTO_UNIQUE_UNLOCK( lock );

		termination_done( );
	}
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
			case termination::process_backlog:
				pimpl_->allow_more_jobs_ = false;
				break;
		}
	}

	pimpl_->cond_.notify_one( );
}

} // namespace q
