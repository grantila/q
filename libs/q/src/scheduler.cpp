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
	: mutex_( std::unique_ptr< q::mutex >(
		new q::mutex( Q_HERE, "circular_list" ) ) )
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

	template< typename Cond >
	T find_first( Cond&& cond )
	{
		Q_AUTO_UNIQUE_LOCK( *mutex_ );

		if ( list_.empty( ) )
			return T( );

		auto here = next_;

		do
		{
			if ( Cond( *next_ ) )
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
	auto _this = shared_from_this( );

	auto runner = [ _this ]( ) mutable
	{
		// TODO: Ensure this doesn't throw...
		auto t = _this->next_task( );
		if ( !!t )
			t( );
	};

	pimpl_->event_dispatcher_->add_task( std::move( runner ) );
}

task scheduler::next_task( )
{
	task ret = pimpl_->queues_.pop_next( );
	return std::move( ret );
}

} // namespace q
