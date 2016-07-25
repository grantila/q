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

#include <q/types.hpp>
#include <q/exception.hpp>
#include <q/detail/lib.hpp>

#include <atomic>

#ifdef _WIN32
#	include <q/detail/platform/windows.hpp>
	EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#elif defined( __linux__ )
#	ifndef _GNU_SOURCE
#		define _GNU_SOURCE
#	endif
#	include <dlfcn.h>
#	include <linux/limits.h>
#else // posix
#	include <dlfcn.h>
#endif

#ifdef _WIN32
#	define EXPORT __declspec(dllexport)
#	ifndef PATH_MAX
#		define PATH_MAX _MAX_PATH
#	endif
#else
#	define EXPORT
#endif

namespace {

#ifndef _WIN32
	// TODO: Make non-optimized and garuanteed-to-exist and have a pointer to it
	extern "C" EXPORT int silly_function( void ) { return 0; }
#endif

static const char* get_shared_object_name( )
noexcept
{
	static char filename[ PATH_MAX ] = { 0 };

	// TODO: Apply proper locking

	if ( filename[ 0 ] )
		return filename;

#ifdef _WIN32
	::GetModuleFileName( ( HINSTANCE )&__ImageBase, filename, PATH_MAX );
#else // not _WIN32

	silly_function( );
	Dl_info info = { 0 };
	dladdr( __builtin_return_address( 0 ), &info);
	snprintf( filename, PATH_MAX, "%s", info.dli_fname );
#endif
	return filename;
}

static class init_dummy
{
public:
	init_dummy( )
	noexcept
	{
		shared_object_ = get_shared_object_name( );
	}
	~init_dummy( )
	{ }

	const char* shared_object( ) const
	noexcept
	{
		return shared_object_;
	}

private:
	const char* shared_object_;
} init_dummy_;

std::atomic< ::q::detail::uncaught_exception_handler_fn > _uncaught_exception;

} // anonymous namespace

namespace q { namespace detail {

void register_uncaught_exception_handler( uncaught_exception_handler_fn e )
noexcept
{
	_uncaught_exception.store( e, std::memory_order_seq_cst );
}

void handle_uncaught_exception( const std::exception_ptr& e )
noexcept
{
	auto fn = _uncaught_exception.load( std::memory_order_seq_cst );

	try
	{
		if ( fn )
		{
			fn( e );
		}
		else
		{
			std::cerr
				<< "q caught an exception not caught by the "
				<< "application:" << std::endl
				<< stream_exception( e );
			std::terminate( );
		}
	}
	catch ( ... )
	{
		auto e2 = std::current_exception( );
		std::cerr
			<< "q: The registered 'uncaught exception handler' "
			<< "threw an exception: " << std::endl
			<< stream_exception( e2 ) << std::endl
			<< "when it got the following exception: " << std::endl
			<< stream_exception( e ) << std::endl;
		std::terminate( );
	}
}

const char* shared_object( )
noexcept
{
	return init_dummy_.shared_object( );
}

} } // namespace detail, namespace q
	
