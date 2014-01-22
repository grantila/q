
#include <q/execution_context.hpp>
#include <q/scheduler.hpp>

namespace q {

struct execution_context::pimpl
{
	event_dispatcher_ptr event_dispatcher_;
	queue_ptr queue_;
	scheduler_ptr scheduler_;
};

execution_context::execution_context( event_dispatcher_ptr ed )
: pimpl_( new pimpl )
{
	pimpl_->event_dispatcher_ = ed;
	pimpl_->queue_ = q::make_shared< q::queue >( );
	pimpl_->scheduler_ = q::make_shared< q::scheduler >( ed );

	pimpl_->scheduler_->add_queue( pimpl_->queue_ );
}

execution_context::~execution_context( )
{ }

queue_ptr execution_context::queue( )
{
	return pimpl_->queue_;
}

} // namespace q
