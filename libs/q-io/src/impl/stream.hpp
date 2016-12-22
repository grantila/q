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

#ifndef LIBQIO_INTERNAL_IMPL_STREAM_HPP
#define LIBQIO_INTERNAL_IMPL_STREAM_HPP

#include "handle.hpp"

namespace q { namespace io {

struct stream
: handle
{
	::uv_stream_t* uv_stream( )
	{
		return reinterpret_cast< ::uv_stream_t* >( handle_ );
	}

	stream( ) = delete;
	stream( ::uv_stream_t* s )
	: handle( reinterpret_cast< ::uv_handle_t* >( s ) )
	{ }
};

} } // namespace io, namespace q

#endif // LIBQIO_INTERNAL_IMPL_STREAM_HPP
