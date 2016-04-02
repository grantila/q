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

#include <qurl/context.hpp>
#include <qurl/handle.hpp>

#include "internals.hpp"

#include <q-io/socket_event.hpp>
#include <q-io/timeout_event.hpp>


namespace qurl {

context::context( ::q::io::dispatcher_ptr dispatcher )
: pimpl_( q::make_unique< pimpl >( ) )
{
	pimpl_->dispatcher_ = std::move( dispatcher );
	pimpl_->multi_handle_ = curl_multi_init( );
	if ( pimpl_->multi_handle_ == nullptr )
		Q_THROW( q::exception( ) );

	pimpl_->weak_context_socket_ = nullptr;
	pimpl_->weak_context_timeout_ = nullptr;
}

context::~context( )
{
	// TODO: Remove all easy handles
	// CURLMcode curl_multi_remove_handle(CURLM *multi_handle, CURL *easy_handle);

	::curl_multi_setopt(
		pimpl_->multi_handle_, CURLMOPT_SOCKETFUNCTION, nullptr );
	::curl_multi_setopt(
		pimpl_->multi_handle_, CURLMOPT_SOCKETDATA, nullptr );
	::curl_multi_setopt(
		pimpl_->multi_handle_, CURLMOPT_TIMERFUNCTION, nullptr );
	::curl_multi_setopt(
		pimpl_->multi_handle_, CURLMOPT_TIMERDATA, nullptr );

	::curl_multi_cleanup( pimpl_->multi_handle_ );

	if ( pimpl_->weak_context_socket_ )
		delete pimpl_->weak_context_socket_;
	if ( pimpl_->weak_context_timeout_ )
		delete pimpl_->weak_context_timeout_;
}

context_ptr context::construct( ::q::io::dispatcher_ptr dispatcher )
{
	auto ctx = q::make_shared_using_constructor< context >(
		std::move( dispatcher ) );

	ctx->_init( );

	return ctx;
}

handle_ptr context::create_handle( )
{
	return handle::construct( shared_from_this( ) );
}

void context::_init( )
{
	auto self = shared_from_this( );

	pimpl_->timeout_ = curl_timeout::construct( self );

	pimpl_->weak_context_socket_ = new context_weak_ptr( self );
	pimpl_->weak_context_timeout_ = new context_weak_ptr( self );

	typedef int ( *socket_callback_func )(
		CURL*, curl_socket_t, int, void*, void* );

	socket_callback_func socket_callback = [ ](
		CURL* easy,
		curl_socket_t sock,
		int what,
		void* userp,
		void* socketp
	)
	-> int
	{
		auto ctx_ptr = reinterpret_cast< context_weak_ptr* >( userp );

		if ( !ctx_ptr )
			return 0;

		auto self = ctx_ptr->lock( );

		// This callback should practically only be called from the
		// same thread (from libevent2 callbacks), so we won't need
		// locking around the map.

		auto& sock_map = self->pimpl_->sockets_;
		curl_socket_ptr sock_ev;

		auto sock_iter = sock_map.find( sock );

		if ( sock_iter == sock_map.end( ) )
		{
			sock_ev = std::make_shared< curl_socket >( self, sock );

			sock_map[ sock ] = sock_ev;

			self->pimpl_->dispatcher_->attach_event( sock_ev );
		}
		else
		{
			sock_ev = sock_iter->second;
		}

		switch ( what )
		{
			case CURL_POLL_IN:
				sock_ev->detect_readability( );
				break;
			case CURL_POLL_OUT:
				sock_ev->detect_writability( );
				break;
			case CURL_POLL_INOUT:
				sock_ev->detect_readability( );
				sock_ev->detect_writability( );
				break;
			case CURL_POLL_REMOVE:
				sock_map.erase( sock );
				break;
			default:
				;// TODO: Warn or something
		}

		return 0;
	};

	CURLMcode ret_socket_function = curl_multi_setopt(
		pimpl_->multi_handle_,
		CURLMOPT_SOCKETFUNCTION,
		socket_callback );

	if ( ret_socket_function != CURLM_OK )
	{
		;
	}

	CURLMcode ret_socket_data = curl_multi_setopt(
		pimpl_->multi_handle_,
		CURLMOPT_SOCKETDATA,
		pimpl_->weak_context_socket_ );

	if ( ret_socket_data != CURLM_OK )
	{
		;
	}

	typedef int( *timer_callback_func )( CURLM*, long, void* );

	timer_callback_func timer_callback = [ ](
		CURLM* multi, long timeout_ms, void* userp
	)
	-> int
	{
		auto ctx_ptr = reinterpret_cast< context_weak_ptr* >( userp );

		if ( !ctx_ptr )
			return 0;

		auto self = ctx_ptr->lock( );

		if ( !self )
			return 0;

		if ( timeout_ms == -1 )
			self->pimpl_->timeout_->remove_timeout( );
		else if ( timeout_ms == 0 )
			self->pimpl_->timeout_->set_timeout_now( );
		else
			self->pimpl_->timeout_->set_timeout(
				std::chrono::milliseconds( timeout_ms ) );

		return 0;
	};

	CURLMcode ret_timer_function = curl_multi_setopt(
		pimpl_->multi_handle_, CURLMOPT_TIMERFUNCTION, timer_callback );

	if ( ret_timer_function != CURLM_OK )
	{
		;
	}

	CURLMcode ret_timer_data = curl_multi_setopt(
		pimpl_->multi_handle_,
		CURLMOPT_TIMERDATA,
		pimpl_->weak_context_timeout_ );

	if ( ret_timer_data != CURLM_OK )
	{
		;
	}
}

void context::on_readable( ::q::io::socket_t sock ) noexcept
{
	int running_handles;
	::curl_multi_socket_action(
		pimpl_->multi_handle_,
		sock,
		CURL_CSELECT_IN,
		&running_handles );
}

void context::on_writable( ::q::io::socket_t sock ) noexcept
{
	int running_handles;
	::curl_multi_socket_action(
		pimpl_->multi_handle_,
		sock,
		CURL_CSELECT_OUT,
		&running_handles );
}

void context::on_timeout( ) noexcept
{
	int running_handles;
	::curl_multi_socket_action(
		pimpl_->multi_handle_,
		CURL_SOCKET_TIMEOUT,
		0,
		&running_handles );
}

} // namespace qurl
