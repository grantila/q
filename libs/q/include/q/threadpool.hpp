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

#ifndef LIBQ_THREADPOOL_HPP
#define LIBQ_THREADPOOL_HPP

#include <q/async_termination.hpp>
#include <q/event_dispatcher.hpp>
#include <q/thread.hpp>

namespace q {

class threadpool
: public event_dispatcher
, public async_termination< >
, public std::enable_shared_from_this< threadpool >
{
public:
	~threadpool( );

	void add_task( task task ) override;

	static std::shared_ptr< threadpool >
	construct( const std::string& name,
	           std::size_t threads = hard_cores( ) );

protected:
	threadpool( const std::string& name, std::size_t threads );

private:
	void start( );

	std::size_t backlog( ) const override;

	void mark_completion( );
	void do_terminate( ) override;

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

} // namespace q

#endif // LIBQ_THREADPOOL_HPP
