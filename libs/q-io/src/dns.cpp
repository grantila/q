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

#include <q-io/dns.hpp>

#include <q/threadpool.hpp>

#include "internals.hpp"

namespace q { namespace io {

#if 0

#ifdef QIO_USE_LIBEVENT

resolver::resolver( dispatcher_ptr dispatcher, settings settings )
: pimpl_( new pimpl{ dispatcher, nullptr } )
{
	int flags = 0;

	if ( !Q_ENUM_HAS( settings, resolver::settings::no_local_overrides ) )
		flags |= EVDNS_BASE_INITIALIZE_NAMESERVERS;

	if ( !Q_ENUM_HAS( settings, resolver::settings::force_dispatcher_alive ) )
		flags |= EVDNS_BASE_DISABLE_WHEN_INACTIVE;

	pimpl_->base_ = ::evdns_base_new(
		dispatcher->pimpl_->event_base, flags );
}

resolver::~resolver( )
{ }

// TODO: Make exceptions out of these:
/*
#define 	DNS_ERR_CANCEL   69
 	The request was canceled via a call to evdns_cancel_request. 
#define 	DNS_ERR_FORMAT   1
 	The name server was unable to interpret the query. 
#define 	DNS_ERR_NODATA   70
 	There were no answers and no error condition in the DNS packet. 
#define 	DNS_ERR_NONE   0
 	Error codes 0-5 are as described in RFC 1035. 
#define 	DNS_ERR_NOTEXIST   3
 	The domain name does not exist. 
#define 	DNS_ERR_NOTIMPL   4
 	The name server does not support the requested kind of query. 
#define 	DNS_ERR_REFUSED   5
 	The name server refuses to reform the specified operation for policy reasons. 
#define 	DNS_ERR_SERVERFAILED   2
 	The name server was unable to process this query due to a problem with the name server. 
#define 	DNS_ERR_SHUTDOWN   68
 	The request was canceled because the DNS subsystem was shut down. 
#define 	DNS_ERR_TIMEOUT   67
 	Communication with the server timed out. 
#define 	DNS_ERR_TRUNCATED   65
 	The reply was truncated or ill-formatted. 
#define 	DNS_ERR_UNKNOWN   66
 	An unknown error occurred.
*/

q::promise< std::tuple< resolver_response > >
resolver::lookup(
	q::queue_ptr queue, const std::string& name, resolve_flags flags )
{
	int _flags = 0;

	if ( Q_ENUM_HAS( flags, resolve_flags::no_dns_search ) )
		_flags |= DNS_QUERY_NO_SEARCH;

	auto make_resolver = [ this, &name, _flags, &queue ]( bool ipv4 )
	-> q::promise< std::tuple< resolver_response > >
	{
		return q::make_promise_sync( queue, [ this, &name, _flags, ipv4 ](
			q::resolver< resolver_response > resolve,
			q::rejecter< resolver_response > reject )
		{
			struct user_struct
			{
				q::resolver< resolver_response > resolve;
				q::rejecter< resolver_response > reject;
				std::shared_ptr< resolver > ref;
			};

			auto userptr = new user_struct
			{
				resolve,
				reject,
				shared_from_this( )
			};

			auto cb = [ ](
				int result,
				char type,
				int count,
				int ttl,
				void* addresses,
				void* arg )
			{
				std::unique_ptr< user_struct > user(
					reinterpret_cast< user_struct* >(
						arg ) );

				if ( result != DNS_ERR_NONE )
				{
					// TODO: Fix this, make proper
					//       exception classes
					user->reject( q::exception( ) );
				}

				resolver_response response;
				response.ttl = std::chrono::seconds( ttl );

				if ( type == DNS_IPv4_A )
				{
					response.ips.ipv4.resize( count );
					memcpy(
						&response.ips.ipv4[ 0 ],
						addresses,
						count * 4 );
				}
				else if ( type == DNS_IPv6_AAAA )
				{
					response.ips.ipv6.resize( count );
					memcpy(
						&response.ips.ipv6[ 0 ],
						addresses,
						count * 16 );
				}

				user->resolve( std::move( response ) );
			};

			::evdns_request* request = ipv4
				? ::evdns_base_resolve_ipv4(
					pimpl_->base_,
					name.c_str( ),
					_flags,
					cb,
					reinterpret_cast< void* >( userptr ) )
				: ::evdns_base_resolve_ipv6(
					pimpl_->base_,
					name.c_str( ),
					_flags,
					cb,
					reinterpret_cast< void* >( userptr ) );

			if ( !request )
			{
				// TODO: Fix this, make proper exception classes
				reject( q::exception( ) );
			}
		} );
	};

	auto normal = Q_ENUM_HAS( flags, resolve_flags::normal );

	auto await_ipv4 = ( Q_ENUM_HAS( flags, resolve_flags::ipv4 ) || normal )
		? make_resolver( true )
		: q::with( queue, resolver_response( ) );

	auto await_ipv6 = ( Q_ENUM_HAS( flags, resolve_flags::ipv6 ) || normal )
		? make_resolver( false )
		: q::with( queue, resolver_response( ) );

	return q::all( std::move( await_ipv4 ), std::move( await_ipv6 ) )
	.then( [ ]( resolver_response&& ipv4, resolver_response&& ipv6 )
	-> resolver_response
	{
		ipv4.ips.ipv6 = std::move( ipv6.ips.ipv6 );
		ipv4.ttl = std::min( ipv4.ttl, ipv6.ttl );
		return ipv4;
	} );
}

#endif

#endif // 0

} } // namespace io, namespace q
