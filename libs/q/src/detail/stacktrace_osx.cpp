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

#include <q/pp.hpp>

#ifdef LIBQ_ON_OSX

#include "stacktrace.hpp"

namespace q {

namespace detail {

/*
 Example line:
 0   libq.dylib  0x0000000100110015 q::(anonymous namespace)::default_stacktrace() + 69
 */
stacktrace::frame parse_stack_frame( const char* data )
noexcept
{
	stacktrace::frame frame;

	frame.frameno = std::strtoul( data, nullptr, 10 );

	const char* pos, *pos2;

	pos = std::strchr( data, ' ' );
	if ( pos )
	{
		pos += std::strspn( pos, " " );
		pos2 = std::strchr( pos, ' ' );

		if ( pos2 )
			// On OSX, backtrace_symbols return string is modifyable
			const_cast< char& >( pos2[ 0 ] ) = 0;
		auto names = get_file_name( pos );
		frame.lib = std::move( names.first );
		frame.lib_path = std::move( names.second );

		pos = pos2 + 1;
	}

	if ( pos )
	{
		pos += std::strspn( pos, " " );
		pos2 = std::strchr( pos, ' ' );

		if ( pos[ 0 ] == '0' && pos[ 1 ] == 'x' )
			frame.addr = hex_to_dec( pos + 2 );

		pos = pos2;
	}

	if ( pos )
	{
		pos += std::strspn( pos, " " );
		pos2 = std::strchr( pos, ' ' );

		if ( pos2 )
			frame.symbol.append( pos, pos2 - pos );
		else
			frame.symbol.append( pos );
		pos = pos2;
	}

	if ( pos )
	{
		pos += std::strspn( pos, " " );
		
		frame.extra.append( pos );
	}

	return frame;
}

}
	
}

#endif
