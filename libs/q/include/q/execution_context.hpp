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

#ifndef LIBQ_EXECUTION_CONTEXT_HPP
#define LIBQ_EXECUTION_CONTEXT_HPP

#include <q/types.hpp>
#include <q/event_dispatcher.hpp>

#include <memory>

namespace q {

class execution_context;
typedef std::shared_ptr< execution_context > execution_context_ptr;

class execution_context
{
public:
	~execution_context( );

	queue_ptr queue( );

protected:
	execution_context( event_dispatcher_ptr ed );

private:
	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

template< typename EventDispatcher, typename... Args >
execution_context_ptr make_execution_context( Args&&... args )
{
	auto ed = q::make_shared< EventDispatcher >(
		std::forward< Args >( args )... );
	return q::make_shared< execution_context >( ed );
}

} // namespace q

#endif // LIBQ_EXECUTION_CONTEXT_HPP
