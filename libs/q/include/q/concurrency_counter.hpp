/*
 * Copyright 2016 Gustaf Räntilä
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

#ifndef LIBQ_CONCURRENCY_COUNTER_HPP
#define LIBQ_CONCURRENCY_COUNTER_HPP

#include <q/concurrency.hpp>
#include <q/promise.hpp>
#include <q/mutex.hpp>

namespace q {

class concurrency_counter
{
	struct continuation
	{
		std::shared_ptr< detail::defer< > > defer_;
		shared_promise< std::tuple< > > promise_;

		continuation(
			std::shared_ptr< detail::defer< > >&& defer,
			shared_promise< std::tuple< > >&& promise
		)
		: defer_( std::move( defer ) )
		, promise_( std::move( promise ) )
		{ }
	};

public:
	concurrency_counter( queue_ptr queue, std::size_t limit )
	: mut_( "concurrency_helper" )
	, queue_( std::move( queue ) )
	, limit_( limit )
	, value_( 0 )
	{ }

	void inc( )
	{
		Q_AUTO_UNIQUE_LOCK( mut_ );

		++value_;
	}

	void dec( )
	{
		std::shared_ptr< continuation > _continuation;

		{
			Q_AUTO_UNIQUE_LOCK( mut_ );

			if ( value_ == 0 )
				Q_THROW( domain_error( "counter already zero" ) );

			if ( value_ == limit_ )
			{
				_continuation = continuation_;
				continuation_.reset( );
			}

			--value_;
		}

		if ( _continuation )
			_continuation->defer_->set_value( );
	}

	promise< std::tuple< > > get_promise( )
	{
		Q_AUTO_UNIQUE_LOCK( mut_ );

		if ( value_ < limit_ )
			return q::with( queue_ );

		if ( !continuation_ )
		{
			auto defer = detail::defer< >::construct( queue_ );

			auto prom = defer->get_promise( ).share( );

			continuation_ = std::make_shared< continuation >(
				std::move( defer ), std::move( prom ) );
		}

		return continuation_->promise_.unshare( );
	}

private:
	mutex mut_;
	queue_ptr queue_;
	std::shared_ptr< continuation > continuation_;
	std::size_t limit_;
	std::size_t value_;
};

} // namespace q

#endif // LIBQ_CONCURRENCY_COUNTER_HPP
