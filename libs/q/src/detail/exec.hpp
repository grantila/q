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

#include <q/pp.hpp>

#ifdef LIBQ_ON_LINUX

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string>
#include <string.h>

namespace q {

namespace detail {

std::string exec_read_all( const std::string& prog, std::size_t bytes = 4096 )
{
	std::string ret( bytes, '\0' );

	int stdout_pipe[ 2 ];

	if ( ::pipe( stdout_pipe ) < 0 )
		return "";

	pid_t pid;

	switch( pid = ::fork( ) )
	{
		case -1:
			::close( stdout_pipe[ 0 ] );
			::close( stdout_pipe[ 1 ] );
			return "";
		case 0:
		{
			// Child
			::close( 0 ); // Close stdin
			::close( 1 ); // Close stdout
			::close( 2 ); // Close stderr

			::close( stdout_pipe[ 0 ] );
			::dup2( stdout_pipe[ 1 ], 1 );

			char* const args[ 2 ] = {
				::strdup( prog.c_str( ) ),
				nullptr
			};
			::execvp( args[ 0 ], args );
			// We shouldn't end up here

			::close( stdout_pipe[ 1 ] );
			_exit( 1 );
		}
		default:
			// Parent
			::close( stdout_pipe[ 1 ] );
			auto count = ::read(
				stdout_pipe[ 0 ],
				const_cast< char* >( ret.data( ) ),
				bytes - 1 );
			ret.erase( count );
			::close( stdout_pipe[ 0 ] );
			return ret;
	}

	return "";
}

} // namespace detail

} // namespace q

#endif // LIBQ_ON_LINUX
