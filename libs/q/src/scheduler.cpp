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
#include <forward_list>

namespace q {

template< typename T >
class circular_list
{
public:
	circular_list( )
	{
		next_ = list_.begin( );
	}

	void add( T&& t )
	{
		auto set_next = empty( );

		list_.push_front( std::move( t ) );

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
	T* next( )
	{
		if ( list_.empty( ) )
			return nullptr;

		auto ret = next_;

		traverse( );

		return &*ret;
	}

	template< typename Cond >
	T* find_first( Cond&& cond )
	{
		if ( list_.empty( ) )
			return nullptr;

		auto here = next_;

		do
		{
			if ( Cond( *next_ ) )
			{
				auto ret = next_;

				traverse( );

				return &*ret;
			}

			traverse( );
		} while ( here != next_ );

		return nullptr;
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

	std::forward_list< T > list_;
	typename std::forward_list< T >::iterator next_;
};

template< typename P, typename T >
class round_robin_priority_list
{
public:
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
			element_type element{ std::move( priority ) };
			iter = list_.insert( iter, std::move( element ) );
		}

		iter->circular_list_.add( std::move( elem ) );
	}

	void remove( P&& priority, T&& elem )
	{
		// TODO: Implement
	}

	template< typename Cond >
	T* find_first( Cond&& condition )
	{
		for ( auto& elem : list_ )
		{
			if ( !elem.circular_list_.empty( ) )
			{
				auto pelem = elem.circular_list_.find_first( condition );
				if ( pelem )
					return pelem;
			}
		}

		return nullptr;
	}

private:
	struct element_type
	{
		P priority_;
		circular_list< T > circular_list_;

		operator P( ) const
		{
			return priority_;
		}
	};

	std::vector< element_type > list_;
};

struct scheduler::pimpl
{
	pimpl( event_dispatcher_ptr event_dispatcher )
	: event_dispatcher_( event_dispatcher )
	{ }

	event_dispatcher_ptr event_dispatcher_;
	round_robin_priority_list< priority_t, queue_ptr > queues_;
};


scheduler::scheduler( event_dispatcher_ptr event_dispatcher )
: pimpl_( new pimpl( event_dispatcher ) )
{ }

scheduler::~scheduler( )
{ }

void scheduler::add_queue( queue_ptr queue )
{
	pimpl_->queues_.add( queue->priority( ), queue_ptr( queue ) );

	auto backlog = queue->set_consumer( std::bind( &scheduler::poke, this ) );

	for ( auto i = backlog; i > 0; --i )
		poke( );
}

void scheduler::poke( )
{
	auto This = shared_from_this( );

	auto runner = [ This ]( ) mutable
	{
		// TODO: Ensure this doesn't throw...
		auto t = This->next_task( );
		t( );
	};

	pimpl_->event_dispatcher_->add_task( std::move( runner ) );
}

task scheduler::next_task( )
{
	auto condition = [ ]( queue_ptr queue )
	{
		return !queue->empty( );
	};

	auto pqueue = pimpl_->queues_.find_first( condition );

	if ( !pqueue )
		throw 0; // TODO: Throw something good here as this should never happen! Or allow this to happen if it helps building a lock-free queue.

	task ret = ( *pqueue )->pop( );
	return std::move( ret );
}

} // namespace q
