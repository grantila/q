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

#ifndef LIBQ_SCHEDULER_HPP
#define LIBQ_SCHEDULER_HPP

#include <q/event_dispatcher.hpp>
#include <q/queue.hpp>

#include <memory>

namespace q {

class scheduler;
typedef std::shared_ptr< scheduler > scheduler_ptr;

class scheduler
: public std::enable_shared_from_this< scheduler >
{
public:
	scheduler( const scheduler& ) = delete;
	scheduler( scheduler&& ) = delete;
	~scheduler( );

	void add_queue( queue_ptr queue );

protected:
	scheduler( event_dispatcher_ptr event_dispatcher );

private:
	void poke( );
	task next_task( );

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

} // namespace q

#endif // LIBQ_SCHEDULER_HPP
