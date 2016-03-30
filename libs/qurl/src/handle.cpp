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

#include <qurl/handle.hpp>

#include "internals.hpp"

#include <q/memory.hpp>

namespace qurl {

struct header_data
{
	std::string in;
	std::string out;
};

struct handle::pimpl
{
	context_ptr context_;
	CURL* curl_handle_;
	header_data headers;
};

handle::handle( context_ptr context )
: pimpl_( q::make_unique< pimpl >( ) )
{
	pimpl_->context_ = std::move( context );
	pimpl_->curl_handle_ = ::curl_easy_init( );
}

handle::~handle( )
{
	::curl_multi_remove_handle(
		pimpl_->context_->pimpl_->multi_handle_,
		pimpl_->curl_handle_ );

	::curl_easy_cleanup( pimpl_->curl_handle_ );
}

handle_ptr handle::construct( context_ptr context )
{
	auto self = q::make_shared_using_constructor< handle >( context );

	auto userp = new handle_weak_ptr( self );

	auto curl = self->pimpl_->curl_handle_;

	::curl_multi_add_handle( context->pimpl_->multi_handle_, curl );

	CURLcode ret = ::curl_easy_setopt( curl, CURLOPT_PRIVATE, userp );
	if ( ret != CURLE_OK )
	{
		delete userp;
		Q_THROW( exception( ) );
	}

	return self;
}

void handle::get_stuff( )
{
	auto self = shared_from_this( );
	auto curl = pimpl_->curl_handle_;

	CURLcode res;
	//::curl_easy_setopt( curl, CURLOPT_URL, "http://bbc.com" );
	::curl_easy_setopt( curl, CURLOPT_URL, "http://google.com" );

	auto this_ptr = new handle_ptr( self );

	typedef size_t( *write_function )(
		char* bufptr, size_t size, size_t nmemb, void* userdata );

	auto writer = [ ](
		char* bufptr, size_t size, size_t nmemb, void* userdata
	)
	-> size_t
	{
		//auto this_ptr = reinterpret_cast< handle_ptr* >( userp );

		std::size_t nbytes = size * nmemb;

		std::cout << "GOT DATA " << nbytes << " BYTES:" << std::string( (char*)bufptr, nbytes ) << std::endl;

		return nbytes;

		//delete this_ptr;
	};

	write_function w = writer;

	::curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, w );
	::curl_easy_setopt( curl, CURLOPT_WRITEDATA, this_ptr );

	typedef int( *debug_function )( ::CURL*, ::curl_infotype, char*, ::size_t, void* );

	auto debug_this_ptr = new handle_weak_ptr( self );

	auto debug_cb = [ ]( ::CURL* handle, ::curl_infotype type, char* data, ::size_t size, void* userdata )
	-> int
	{
		handle_weak_ptr* self_ptr = reinterpret_cast< handle_weak_ptr* >( userdata );
		handle_ptr self = self_ptr->lock( );

		if ( !self )
			return 0;

		header_data& _headers = self->pimpl_->headers;

		switch ( type )
		{
			case CURLINFO_HEADER_IN:
				_headers.in = std::string( data, size );
				break;
			case CURLINFO_HEADER_OUT:
				_headers.out = std::string( data, size );
				break;
			default:
				break;
		}

		return 0;
	};

	debug_function dbg_func = debug_cb;

	::curl_easy_setopt( curl, CURLOPT_VERBOSE, 1 );
	::curl_easy_setopt( curl, CURLOPT_DEBUGFUNCTION, dbg_func );
	::curl_easy_setopt( curl, CURLOPT_DEBUGDATA, debug_this_ptr );

	::curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1 );

	pimpl_->context_->pimpl_->timeout_->set_timeout_now( );
//	res = ::curl_easy_perform( curl );
}

} // namespace qurl
