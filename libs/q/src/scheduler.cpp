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

#include <q/scheduler.hpp>

#include <vector>

namespace q {

/**
 * Circular list.
 *
 * This container can be iterated forward forever, by circulating over the same
 * underlying list of elements. Useful for round-robin distribution.
 */
template< typename T >
class circular_list
{
public:
	circular_list( )
	// TODO: Use q::make_unique
	: mutex_( q::make_unique< q::mutex >( Q_HERE, "circular_list" ) )
	{
		next_ = list_.begin( );
	}

	void add( T&& t )
	{
		Q_AUTO_UNIQUE_LOCK( *mutex_ );

		auto set_next = empty( );

		list_.push_back( std::move( t ) );

		if ( set_next )
			next_ = list_.begin( );
	}

	void remove( T&& t )
	{
		; // TODO: Implement
	}

	/**
	 * Returns a pointer to the "next" element, or nullptr if the circular
	 * list is empty.
	 */
	T next( )
	{
		Q_AUTO_UNIQUE_LOCK( *mutex_ );

		if ( list_.empty( ) )
			return T( );

		auto ret = next_;

		traverse( );

		return ret;
	}

	T find_first( bool( cond )( const queue_ptr& queue ) )
	{
		Q_AUTO_UNIQUE_LOCK( *mutex_ );

		if ( list_.empty( ) )
			return T( );

		auto here = next_;

		do
		{
			if ( cond( *next_ ) )
			{
				auto ret = next_;

				traverse( );

				return *ret;
			}

			traverse( );
		} while ( here != next_ );

		return T( );
	}

	bool empty( ) const
	{
		return list_.empty( );
	}

private:
	void traverse( )
	{
		if ( ++next_ == list_.end( ) )
			next_ = list_.begin( );
	}

	std::unique_ptr< q::mutex > mutex_;
	std::vector< T > list_;
	typename std::vector< T >::iterator next_;
};

template< typename T >
struct deduct_element_type
{
	typedef typename T::element_type type;
};
template< typename T >
struct deduct_element_type< std::shared_ptr< T > >
{
	typedef typename T::element_type type;
};

template< typename P, typename T >
class round_robin_priority_list
{
public:
	typedef typename deduct_element_type< T >::type element_type;

	round_robin_priority_list( ) = default;

	void add( P&& priority, T&& elem )
	{
		auto iter = std::lower_bound(
			list_.begin( ),
			list_.end( ),
			priority );

		if ( iter == list_.end( ) || iter->priority_ != priority )
		{
			// Insert new unique priority circular list
			list_element_type element{ std::move( priority ) };
			iter = list_.insert( iter, std::move( element ) );
		}

		iter->circular_list_.add( std::move( elem ) );
	}

	void remove( P&& priority, T&& elem )
	{
		// TODO: Implement
	}

	element_type pop_next( )
	{
		for ( auto& elem : list_ )
		{
			if ( !elem.circular_list_.empty( ) )
			{
				auto condition = [ ]( const queue_ptr& queue )
				-> bool
				{
					return !queue->empty( );
				};

				auto found = elem.circular_list_.find_first(
					condition );

				if ( !!found )
					return found->pop( );
			}
		}

		return element_type( );
	}

private:
	struct list_element_type
	{
		P priority_;
		circular_list< T > circular_list_;

		operator P( ) const
		{
			return priority_;
		}
	};

	std::vector< list_element_type > list_;
};

struct priority_scheduler::pimpl
{
	pimpl( const event_dispatcher_ptr& event_dispatcher )
	: event_dispatcher_( event_dispatcher )
	{ }

	event_dispatcher_ptr event_dispatcher_;
	round_robin_priority_list< priority_t, queue_ptr > queues_;
};


priority_scheduler::priority_scheduler(
	const event_dispatcher_ptr& event_dispatcher )
: pimpl_( new pimpl( event_dispatcher ) )
{ }

priority_scheduler::~priority_scheduler( )
{ }

void priority_scheduler::add_queue( queue_ptr queue )
{
	pimpl_->queues_.add( queue->priority( ), queue_ptr( queue ) );

	event_dispatcher_ptr ed = pimpl_->event_dispatcher_;
	queue->set_consumer( [ ed ]( )
	{
		ed->notify( );
	} );

	auto _this = shared_from_this( );

	auto fetcher = [ _this ]( ) mutable noexcept -> timer_task
	{
		return _this->next_task( );
	};

	pimpl_->event_dispatcher_->set_task_fetcher( fetcher );
}

void priority_scheduler::poke( )
{
	auto _this = shared_from_this( );

	pimpl_->event_dispatcher_->notify( );
}

timer_task priority_scheduler::next_task( )
{
	return pimpl_->queues_.pop_next( );
}


struct direct_scheduler::pimpl
{
	event_dispatcher_ptr event_dispatcher_;
	q::mutex mutex_;
	queue_ptr queue_;
};

direct_scheduler::direct_scheduler(
	const event_dispatcher_ptr& event_dispatcher )
: pimpl_( new pimpl{ event_dispatcher, { Q_HERE, "direct_scheduler" } } )
{ }

direct_scheduler::~direct_scheduler( )
{ }

void direct_scheduler::add_queue( queue_ptr queue )
{
	Q_AUTO_UNIQUE_LOCK( pimpl_->mutex_ );

	if ( !pimpl_->queue_ )
	{
		pimpl_->queue_ = queue;

		event_dispatcher_ptr ed = pimpl_->event_dispatcher_;
		queue->set_consumer( [ ed ]( )
		{
			ed->notify( );
		} );
	}
	else
	{
		Q_THROW( not_unique_exception( ) );
	}

	auto _this = shared_from_this( );

	auto fetcher = [ _this ]( ) mutable noexcept -> timer_task
	{
		return _this->next_task( );
	};

	pimpl_->event_dispatcher_->set_task_fetcher( fetcher );
}

void direct_scheduler::poke( )
{
	pimpl_->event_dispatcher_->notify( );
}

timer_task direct_scheduler::next_task( )
{
	return pimpl_->queue_->pop( );
}

} // namespace q
