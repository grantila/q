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

#ifndef LIBQ_BLOCKING_DISPATCHER_HPP
#define LIBQ_BLOCKING_DISPATCHER_HPP

#include <q/event_dispatcher.hpp>
#include <q/thread.hpp>

namespace q {

class blocking_dispatcher
: public event_dispatcher< arguments< termination > >
, public std::enable_shared_from_this< blocking_dispatcher >
{
public:
	~blocking_dispatcher( );

	void add_task( task task ) override;

	void start( ) override;

	std::size_t backlog( ) const override { return 0; }

protected:
	blocking_dispatcher( const std::string& name );
	blocking_dispatcher( )
	: blocking_dispatcher( "" )
	{ }

private:
	void do_terminate( termination term ) override;

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

} // namespace q

#endif // LIBQ_BLOCKING_DISPATCHER_HPP
