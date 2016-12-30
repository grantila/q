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

#ifndef LIBQIO_INTERNAL_IMPL_HANDLE_HPP
#define LIBQIO_INTERNAL_IMPL_HANDLE_HPP

#include <q-io/dispatcher.hpp>

#include "../uv.hpp"

namespace q { namespace io {

// TODO: Move, and properly implement this for Win32
static inline int uv_error_to_errno( int errnum )
{
	return -errnum;
}

struct handle
{
	::uv_handle_t* handle_;

	::uv_handle_t* uv_handle( )
	{
		return handle_;
	}

	handle( ) = delete;
	handle( ::uv_handle_t* h ) : handle_( h ) { }

	virtual void
	attach_dispatcher( const dispatcher_ptr& dispatcher ) noexcept { }

	void close( )
	{
		close( fulfill< void >( ) );
	}

	void close( std::exception_ptr err )
	{
		close( refuse< void >( std::move( err ) ) );
	}

	virtual void close( q::expect< void > ) = 0;
};

} } // namespace io, namespace q

#endif // LIBQIO_INTERNAL_IMPL_HANDLE_HPP
