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

#ifndef LIBQ_PROMISE_ASYNC_TASK_HPP
#define LIBQ_PROMISE_ASYNC_TASK_HPP

#include <q/expect.hpp>
#include <q/functional.hpp>

namespace q {

/**
 * @c async_task is a class inteded to be used to perform tasks in a promise
 * chain which are executed non-q schedulers and have no return value.
 * Instead they act as parts in a chain which just delays execution to the next
 * worker, which will get the return value from the worker acting before the
 * @c async_task.
 * This means that the async_task cannot alter the value chain between the
 * previous and next promise, although it can throw exceptions.
 */
class async_task
{
public:
	typedef q::unique_function< void( q::expect< > ) noexcept > task;
	typedef q::unique_function< void( task ) > forwarded_task;

	async_task( forwarded_task&& fn )
	: fn_( std::move( fn ) )
	{ }

	async_task( ) = delete;
	async_task( async_task&& ) = default;
	async_task( const async_task& ) = delete;

	async_task& operator=( async_task&& ) = delete;
	async_task& operator=( const async_task& ) = delete;

	template< typename Fn >
	typename std::enable_if<
		Q_IS_FUNCTION( Fn )::value
		and
		std::is_same< Q_RESULT_OF( Fn ), void >::value
		and
		Q_ARGUMENTS_ARE( Fn, q::expect< > )::value
	>::type
	run( Fn&& fn )
	{
		fn_( std::forward< Fn >( fn ) );
	}

private:
	forwarded_task fn_;
};

} // namespace q

#endif // LIBQ_PROMISE_ASYNC_TASK_HPP
