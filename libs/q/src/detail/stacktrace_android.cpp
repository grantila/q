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
#include <q/abi.hpp>

#ifdef LIBQ_ON_ANDROID

#include "stacktrace.hpp"

#include <unwind.h>
#include <dlfcn.h>

namespace q { namespace detail {

stacktrace default_stacktrace( ) noexcept
{
	static const std::size_t buflen = 128;
	struct stack {
		std::size_t size = 0;
		void* addresses[ buflen ];
	};
	stack s;

	_Unwind_Backtrace( [ ]( struct _Unwind_Context* context, void* arg)
	{
		uintptr_t pc = _Unwind_GetIP( context );
		stack* s = static_cast< stack* >( arg );
		if ( pc )
		{
			if ( s->size == buflen )
				return _URC_END_OF_STACK;
			s->addresses[ s->size++ ] =
				reinterpret_cast< void* >( pc );
		}
		return _URC_NO_REASON;
	}, &s );

	std::vector< stacktrace::frame > frames;
	frames.reserve( s.size );

	for ( std::size_t i = 0; i < s.size; ++i )
	{
		stacktrace::frame frame;
		Dl_info info;
		if ( dladdr( s.addresses[ i ], &info ) && info.dli_sname )
			frame.symbol = info.dli_sname;
		frame.addr =
			reinterpret_cast< std::uint64_t >( s.addresses[ i ] );
		frame.frameno = i;
		frames.push_back( frame );
	}

	return stacktrace( std::move( frames ) );
}

} } // namespace detail, namespace q

#endif // LIBQ_ON_ANDROID
