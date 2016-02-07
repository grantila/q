/*
 * Copyright 2014 Gustaf Räntilä
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

#include <p/process.hpp>

#include <sys/types.h>
#include <unistd.h>

namespace p {

struct process::pimpl
{
	pid_t parent_pid;
	pid_t child_pid;
	::q::mutex mutex_;
	std::condition_variable cond_;
};

std::shared_ptr< process > process::construct( const ::q::queue_ptr& queue )
{
	return ::q::make_shared_using_constructor< process >( queue );
}

process::process( const ::q::queue_ptr& queue )
: q::async_termination< q::arguments< >, std::tuple< int > >( queue )
, pimpl_( new pimpl )
{
	pimpl_->parent_pid = ::getpid( );

	auto pid = fork( );
	if ( pid == 0 )
	{
		// Child, goes into permanent run() loop
		pimpl_->child_pid = ::getpid( );
	}
	else
	{
		// Parent, moves along directly
		pimpl_->child_pid = pid;
	}
}

void process::run( )
{
	while ( false )
	{
		;
	}
}

void process::do_terminate( )
{
	termination_done( 127 );
}

} // namespace p
