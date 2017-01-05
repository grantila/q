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

#include "dispatcher.hpp"

#include "../uv.hpp"

namespace q { namespace io {

typedef std::shared_ptr< dispatcher::pimpl > dispatcher_pimpl_ptr;

// TODO: Move, and properly implement this for Win32
static inline int uv_error_to_errno( int errnum )
{
	return -errnum;
}

struct handle
{
	// Members

	dispatcher_pimpl_ptr dispatcher_;
	std::shared_ptr< handle > keep_alive_;
	::uv_handle_t* handle_;

	// Accessors and helpers to be called from any thread

	template< typename Self >
	void o_close( dispatcher_pimpl_ptr dispatcher_pimpl, Self&& self )
	{
		std::weak_ptr< handle > weak_self = self;

		dispatcher_pimpl->internal_queue_->push( [ weak_self ]( )
		{
			auto self = weak_self.lock( );

			if ( self )
				self->i_close( );
		} );
	}

	// Accessors and helpers to be called from the internal thread only

	::uv_handle_t* uv_handle( )
	{
		return handle_;
	}

	handle( ) = delete;
	handle( ::uv_handle_t* h ) : handle_( h )
	{
		handle_->data = reinterpret_cast< void* >( this );
	}

	void i_close_handle( )
	{
		::uv_close( handle_, closer );
	}

	virtual void
	i_attach_dispatcher( const dispatcher_pimpl_ptr& dispatcher ) noexcept
	{ }

	void i_close( )
	{
		i_close( fulfill< void >( ) );
	}

	void i_close( std::exception_ptr err )
	{
		i_close( refuse< void >( std::move( err ) ) );
	}

	virtual void i_close( q::expect< void > ) = 0;

private:
	static void closer( ::uv_handle_t* handle )
	{
		auto self = reinterpret_cast< struct handle* >( handle->data );
		handle->data = nullptr;

		self->keep_alive_.reset( );
	}
};

template< typename Pimpl, typename Handle >
static inline typename std::enable_if<
	std::is_same< Handle, ::uv_handle_t* >::value
	or
	std::is_same< Handle, ::uv_timer_t* >::value
	or
	std::is_same< Handle, ::uv_udp_t* >::value
	or
	std::is_same< Handle, ::uv_stream_t* >::value
	or
	std::is_same< Handle, ::uv_stream_t* >::value
	or
	std::is_same< Handle, ::uv_write_t* >::value
	or
	std::is_same< Handle, ::uv_udp_t* >::value,
	Pimpl*
>::type
get_pimpl( Handle handle )
{
	return static_cast< Pimpl* >(
		reinterpret_cast< struct handle* >( handle->data ) );
}

} } // namespace io, namespace q

#endif // LIBQIO_INTERNAL_IMPL_HANDLE_HPP
