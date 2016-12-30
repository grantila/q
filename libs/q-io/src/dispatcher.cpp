/*
 * Copyright 2014 Gustaf Räntilä
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

#include <q-io/dispatcher.hpp>
#include <q-io/config.hpp>
#include <q-io/tcp_socket.hpp>
#include <q-io/server_socket.hpp>

#include "socket_helpers.hpp"
#include "impl/dispatcher.hpp"
#include "impl/handle.hpp"
#include "impl/tcp_socket.hpp"
#include "impl/server_socket.hpp"
#include "impl/timer_task.hpp"

#include <q/queue.hpp>
#include <q/promise.hpp>
#include <q/scope.hpp>
#include <q/threadpool.hpp>

namespace q { namespace io {


std::shared_ptr< dispatcher >
dispatcher::construct( q::queue_ptr user_queue, std::string name )
{
	auto self = q::make_shared_using_constructor< dispatcher >(
		user_queue, name );

	return self;
}

dispatcher::dispatcher( q::queue_ptr user_queue, std::string name )
: event_dispatcher_base( user_queue )
, pimpl_( pimpl::construct( std::move( user_queue ), std::move( name ) ) )
{ }

dispatcher::~dispatcher( )
{ }

// TODO: Remove this, or make libuv-compatible
std::string dispatcher::backend_method( ) const
{
	std::string method;

	return method;
}

promise< std::vector< dispatcher::event_descriptor > >
dispatcher::dump_events( ) const
{
	auto self = shared_from_this( );

	return q::make_promise( get_queue( ), [ self ]( )
	{
		return self->pimpl_->i_dump_events( );
	} );
}

static inline const char* json_bool( bool b, bool with_comma = true )
{
	return b
		? ( with_comma ? "true," : "true" )
		: ( with_comma ? "false," : "false" );
}

promise< std::string > dispatcher::dump_events_json( ) const
{
	return dump_events( )
	.then( [ ]( std::vector< event_descriptor >&& events )
	{
		std::stringstream ss;

		ss << "[";

		for ( auto& desc : events )
		{
			ss << std::endl << "\t{" << std::endl;
			ss << "\t\t\"type\": \"" << desc.type << "\","
				<< std::endl;

			if ( desc.fd != -1 )
				ss << "\t\t\"fd\": " << desc.fd << ","
					<< std::endl;
			if ( !desc.fd_err.empty( ) )
				ss
					<< "\t\t\"error\": \"" << desc.fd_err
					<< "\","
					<< std::endl;
			ss
				<< "\t\t\"active\": "
				<< json_bool( desc.active )
				<< std::endl;
			ss
				<< "\t\t\"closing\": "
				<< json_bool( desc.closing, false )
				<< std::endl;
			ss << "\t},";
		}

		if ( !events.empty( ) )
		{
			ss.seekp( -1, ss.cur );
			ss << std::endl;
		}

		ss << "]";

		return ss.str( );
	} );
}

void dispatcher::start_blocking( )
{
// TODO: Implement error translation
//uv_strerror(int) and uv_err_name(int)

	pimpl_->i_create_loop( );
	pimpl_->i_make_dummy_event( );

	pimpl_->started_ = true;

	if ( pimpl_->deferred_start_ )
	{
		pimpl_->deferred_start_->set_value( );
		pimpl_->deferred_start_.reset( );
	}

	::uv_run( &pimpl_->uv_loop, UV_RUN_DEFAULT );

	pimpl_->stopped_ = true;

	pimpl_->i_cleanup_dummy_event( );

	if ( pimpl_->termination_ == dispatcher_termination::immediate )
	{
		// Stop all events from the inside
		for ( auto& event : pimpl_->i_dump_events( ) )
		{
			auto handle_ptr = reinterpret_cast< ::uv_handle_t* >(
				event.handle );

			if ( handle_ptr->type == ::UV_ASYNC )
				return;

			auto _handle =
				reinterpret_cast< std::shared_ptr< handle >* >(
					handle_ptr->data );

			if ( _handle )
				( *_handle )->close( );
		}
	}

	while ( !pimpl_->i_dump_events( ).empty( ) )
		::uv_run( &pimpl_->uv_loop, UV_RUN_ONCE );

	::uv_loop_close( &pimpl_->uv_loop );

	termination_done( dispatcher_exit::normal );
}

promise< > dispatcher::start( )
{
	auto self = shared_from_this( );

	auto deferred_start = q::make_shared< q::detail::defer< > >(
		pimpl_->user_queue );

	pimpl_->deferred_start_ = deferred_start;

	auto runner = [ self ]( )
	{
		self->start_blocking( );
	};

	pimpl_->thread = q::run( pimpl_->name, pimpl_->user_queue, runner );

	return deferred_start->get_promise( );
}

void dispatcher::notify( )
{
	if ( !pimpl_->started_ )
		return;
	if ( pimpl_->stopped_ )
		return;
	::uv_async_send( &pimpl_->uv_async );
}

void dispatcher::set_task_fetcher( ::q::task_fetcher_task&& fetcher )
{
	pimpl_->task_fetcher_ = std::move( fetcher );
}

q::async_task
dispatcher::delay( q::timer::duration_type dur )
{
	auto self = shared_from_this( );
	auto dur_( std::chrono::duration_cast< clock::duration >( dur ) );

	auto runner = [ dur_, self ]( q::async_task::task fn )
	{
		auto timer = q::make_shared_using_constructor< timer_task >( );

		timer->pimpl_->attach_dispatcher( self );

		auto task = [ fn, timer ]( ) mutable
		{
			timer->unset_task( );

			fn( q::fulfill< void >( ) );
		};

		timer->set_task( std::move( task ) );
		timer->start_timeout( dur_ );
	};

	return q::async_task( runner );
}

q::promise< resolver_response >
dispatcher::lookup( const std::string& name )
{
	return q::make_promise( pimpl_->dns_queue_, [ name ]( ) -> ip_addresses
	{
		struct addrinfo* info;
		int error;

		error = ::getaddrinfo( name.c_str( ), nullptr, nullptr, &info );

		if ( error )
		{
			std::string err = gai_strerror( error );
			Q_THROW( dns_lookup_error( ) ); //, err );
		}

		ip_addresses res;

		struct addrinfo* iter = info;
		while ( iter )
		{
			if ( iter->ai_family == PF_INET )
				res.add( ipv4_address( info->ai_addr ) );

			if ( iter->ai_family == PF_INET6 )
				res.add( ipv6_address( info->ai_addr ) );

			iter = iter->ai_next;
		}

		freeaddrinfo( info );

		return res;
	} )
	.then( [ ]( ip_addresses&& addresses )
	{
		// TTL isn't available in getaddrinfo-based dns lookup.
		return resolver_response{
			std::move( addresses ),
			std::chrono::seconds( 0 )
		};
	} )
	.use_queue( pimpl_->user_queue );
}

// Tries to connect to the first IP address, then the next, etc, until all
// addresses are tried.
q::promise< tcp_socket_ptr > dispatcher::get_tcp_connection(
	ip_addresses&& addr, std::uint16_t port )
{
	struct destination
	{
		ip_addresses addresses;
		std::uint16_t port;

		ip_addresses::iterator cur_addr;
		ip_addresses::iterator end_addr;
		std::shared_ptr< tcp_socket::pimpl > socket_pimpl;
	};

	auto dest = std::make_shared< destination >( );
	dest->addresses = std::move( addr );
	dest->port = port;

	dest->cur_addr = dest->addresses.begin( );
	dest->end_addr = dest->addresses.end( );
	dest->socket_pimpl = q::make_shared< tcp_socket::pimpl >( );

	auto self = shared_from_this( );

	struct context
	{
		std::shared_ptr< context > self;
		dispatcher_ptr dispatcher;
		std::function< void( context* ) > try_connect;
		std::shared_ptr< destination > dest;
		int last_error;
		uv_connect_cb connect_callback;
		ip_address last_address;
		q::resolver< tcp_socket_ptr > resolve_;
		q::rejecter< tcp_socket_ptr > reject_;

		void resolve( tcp_socket_ptr&& socket )
		{
			resolve_( std::move( socket ) );
		}

		void reject( std::exception_ptr&& e )
		{
			auto scope = self;

			reject_( std::move( e ) );

			dest->socket_pimpl->close( );
		}
	};

	uv_connect_cb connect_callback = [ ]( uv_connect_t* req, int status )
	{
		auto ctx = reinterpret_cast< context* >( req->data );

		auto& socket_pimpl = ctx->dest->socket_pimpl;

		if ( status == 0 )
		{
			// Success
			socket_pimpl->attach_dispatcher( ctx->dispatcher );
			auto sock = ::q::io::tcp_socket::construct(
				std::move( socket_pimpl ) );

			ctx->resolve( std::move( sock ) );
		}
		else
		{
			// Error
			ctx->last_error = uv_error_to_errno( status );
			ctx->try_connect( ctx );
		}
	};

	auto try_connect = [ ]( context* ctx )
	{
		auto& socket_pimpl = ctx->dest->socket_pimpl;
		auto& connect = socket_pimpl->connect_;
		auto& socket = socket_pimpl->socket_;

		if ( ctx->dest->cur_addr == ctx->dest->end_addr )
		{
			int _errno = ctx->dest->addresses.ips.empty( )
				? EINVAL
				: ctx->last_error;

			ctx->reject( get_exception_by_errno( _errno ) );
			return;
		}

		ctx->last_address = *ctx->dest->cur_addr++;

		auto addr = ctx->last_address.get_sockaddr( ctx->dest->port );

		::uv_tcp_connect(
			&connect,
			&socket,
			&*addr,
			ctx->connect_callback
		);

		::uv_async_send( &ctx->dispatcher->pimpl_->uv_async );
	};

	auto ctx = std::make_shared< context >( );
	ctx->dispatcher = self;
	ctx->try_connect = try_connect;
	ctx->dest = dest;
	ctx->last_error = ECONNREFUSED;
	ctx->connect_callback = connect_callback;

	return q::make_promise( get_queue( ),
		[ ctx, try_connect ](
			q::resolver< tcp_socket_ptr > resolve,
			q::rejecter< tcp_socket_ptr > reject
		)
	{
		::uv_tcp_init(
			&ctx->dispatcher->pimpl_->uv_loop,
			&ctx->dest->socket_pimpl->socket_
		);

		ctx->self = ctx;

		ctx->resolve_ = resolve;
		ctx->reject_ = reject;

		ctx->dest->socket_pimpl->connect_.data = ctx.get( );

		try_connect( ctx.get( ) );
	} )
	.finally( [ ctx ]( )
	{
		ctx->self.reset( );
	} )
	.use_queue( pimpl_->user_queue );
}

promise< readable< byte_block >, writable< byte_block > >
dispatcher::tcp_connect( std::string hostname, std::uint16_t port )
{
	auto addr = ip_address::from( hostname );
	if ( addr )
	{
		// The hostname looks like a valid IPv4 or IPv6 address, ignore
		// DNS resolution.
		return tcp_connect( addr, port );
	}

	auto self = shared_from_this( );

	return lookup( hostname )
	.then( [ self, port ]( resolver_response&& response )
	{
		return self->tcp_connect( std::move( response.ips ), port );
	}, get_queue( ) )
	.use_queue( pimpl_->user_queue );
}

promise< readable< byte_block >, writable< byte_block > >
dispatcher::tcp_connect( ip_addresses addr, std::uint16_t port )
{
	return get_tcp_connection( addr, port )
	.then( [ ]( tcp_socket_ptr&& socket )
	{
		auto readable = socket->in( );
		auto writable = socket->out( );
		socket->detach( );
		return std::make_tuple(
			std::move( readable ), std::move( writable ) );
	} );
}

q::promise< server_socket_ptr >
dispatcher::listen( std::uint16_t port, ip_addresses&& bind_to )
{
	auto self = shared_from_this( );
	auto server = q::make_shared< server_socket >(
		port, std::move( bind_to ) );

	return q::make_promise( get_queue( ), [ self, server ]( )
	{
		server->pimpl_->attach_dispatcher( self );

		return server;
	} )
	.use_queue( pimpl_->user_queue );
}

void dispatcher::do_terminate( dispatcher_termination termination_method )
{
	termination dns_termination =
		termination_method == dispatcher_termination::graceful
		? termination::linger
		: termination::annihilate;

	pimpl_->termination_ = termination_method;

	pimpl_->dns_context_->dispatcher( )->terminate( dns_termination );

	::uv_stop( &pimpl_->uv_loop );
	::uv_async_send( &pimpl_->uv_async );
}

q::expect< > dispatcher::await_termination( )
{
	pimpl_->dns_context_->dispatcher( )->await_termination( );
	if ( pimpl_->thread )
		return pimpl_->thread->await_termination( );
	return q::fulfill< void >( );
}

} } // namespace io, namespace q
