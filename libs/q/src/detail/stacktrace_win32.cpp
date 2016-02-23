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

#include "stacktrace.hpp"

#include <q/pp.hpp>
#include <q/abi.hpp>

#ifdef LIBQ_ON_WINDOWS

#include <Windows.h>
#include "DbgHelp.h"
#pragma comment( lib, "Dbghelp.lib" )

// In Windows Server 2003 and Windows XP, FramesToSkip + FramesToCapture must
// be less than 63. TODO: Loop and increase FramesToSkip for more modern
// operating systems.
#define MAX_FRAMES_PER_CALL 62

namespace q {

namespace detail {

stacktrace::frame frame_from_address( HANDLE process, void* address )
{
	stacktrace::frame frame;

#define SYMBOL_INFO_NAME_LEN 512
	std::vector< std::uint8_t > symbol_info_buf(
		sizeof( SYMBOL_INFO ) + SYMBOL_INFO_NAME_LEN + 1 );

	SYMBOL_INFO* symbol_info = reinterpret_cast< SYMBOL_INFO* >(
		&symbol_info_buf[ 0 ] );

	symbol_info->MaxNameLen   = SYMBOL_INFO_NAME_LEN;
	symbol_info->SizeOfStruct = sizeof( SYMBOL_INFO );

	auto sym_ok = ::SymFromAddr(
		process, reinterpret_cast< DWORD64 >( address ), 0, symbol_info );

	if ( sym_ok )
	{
		symbol_info->Name[ symbol_info->NameLen ] = 0;
		frame.symbol = q::demangle_cxx( symbol_info->Name );
		// TODO: Fix this, make frame::addr a number
		//frame.addr = symbol_info->Address;
	}

	return frame;
}

stacktrace default_stacktrace( ) noexcept
{
	std::vector< stacktrace::frame > frames;

	void* buf[ MAX_FRAMES_PER_CALL ];

	HANDLE process = ::GetCurrentProcess( );
	::SymInitialize( process, NULL, TRUE );

	auto nframes = ::CaptureStackBackTrace(
		0, MAX_FRAMES_PER_CALL, buf, nullptr );

	for ( std::size_t i = 0; i < nframes; ++i )
	{
		auto frame = frame_from_address( process, buf[ i ] );
		frame.frameno = i;
		frames.push_back( frame );
	}

	return std::move( frames );
}

} // namespace detail

} // namespace q

#endif
