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

#include "unistd.h"

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
, pimpl_( std::make_shared< pimpl >( ) )
{
	pimpl_->user_queue = user_queue;
	pimpl_->name = name;
	pimpl_->dummy_event.pipes[ 0 ] = pimpl_->dummy_event.pipes[ 1 ] = -1;

	pimpl_->dns_context_ = q::make_execution_context< q::threadpool >(
		"q-io dns worker", user_queue, 6 );
	pimpl_->dns_queue_ = pimpl_->dns_context_->queue( );

	// libuv
	::uv_loop_init( &pimpl_->uv_loop );

	_make_dummy_event( );
}

dispatcher::~dispatcher( )
{
}

// TODO: Remove this, or make libuv-compatible
std::string dispatcher::backend_method( ) const
{
	std::string method;

	return method;
}

std::string handle_type_name( ::uv_handle_type type )
{
	switch ( type )
	{
		case ::uv_handle_type::UV_ASYNC: return "async";
		case ::uv_handle_type::UV_CHECK: return "check";
		case ::uv_handle_type::UV_FS_EVENT: return "fs_event";
		case ::uv_handle_type::UV_FS_POLL: return "fs_poll";
		case ::uv_handle_type::UV_HANDLE: return "handle";
		case ::uv_handle_type::UV_IDLE: return "idle";
		case ::uv_handle_type::UV_NAMED_PIPE: return "pipe";
		case ::uv_handle_type::UV_POLL: return "poll";
		case ::uv_handle_type::UV_PREPARE: return "prepare";
		case ::uv_handle_type::UV_PROCESS: return "process";
		case ::uv_handle_type::UV_STREAM: return "stream";
		case ::uv_handle_type::UV_TCP: return "tcp";
		case ::uv_handle_type::UV_TIMER: return "timer";
		case ::uv_handle_type::UV_TTY: return "tty";
		case ::uv_handle_type::UV_UDP: return "udp";
		case ::uv_handle_type::UV_SIGNAL: return "signal";
		case ::uv_handle_type::UV_FILE: return "file";
		case ::uv_handle_type::UV_UNKNOWN_HANDLE: return "{unknown}";
		default: return "{invalid type!}";
	}
};

uv_walk_cb event_walker = [ ]( ::uv_handle_t* handle, void* arg )
{
	typedef std::vector< dispatcher::event_descriptor > arg_type;
	auto events = reinterpret_cast< arg_type* >( arg );

	bool is_fd_backed =
		handle->type == ::uv_handle_type::UV_TCP ||
		handle->type == ::uv_handle_type::UV_NAMED_PIPE ||
		handle->type == ::uv_handle_type::UV_TTY ||
		handle->type == ::uv_handle_type::UV_UDP ||
		handle->type == ::uv_handle_type::UV_POLL;

	uv_os_fd_t fd = -1;
	std::string fd_err;
	if ( is_fd_backed )
	{
		auto ret = ::uv_fileno( handle, &fd );
		if ( ret )
		{
			if ( ret == UV_EINVAL )
				fd_err = "UV_EINVAL";
			else if ( ret == UV_EBADF )
				fd_err = "UV_EBADF";
			else
			{
				std::stringstream ss;
				ss << ret;
				fd_err = ss.str( );
			}
			fd = -1;
		}
	}

	events->push_back( dispatcher::event_descriptor{
		handle,
		handle_type_name( handle->type ),
		static_cast< bool >( ::uv_is_active( handle ) ),
		static_cast< bool >( ::uv_is_closing( handle ) ),
		fd,
		std::move( fd_err )
	} );
};

std::vector< dispatcher::event_descriptor > dispatcher::dump_events( ) const
{
	std::vector< event_descriptor > events;

	::uv_walk( &pimpl_->uv_loop, event_walker, &events );

	return events;
}

static inline const char* json_bool( bool b, bool with_comma = true )
{
	return b
		? ( with_comma ? "true," : "true" )
		: ( with_comma ? "false," : "false" );
}

std::string dispatcher::dump_events_json( ) const
{
	auto events = dump_events( );

	std::stringstream ss;

	ss << "[";

	for ( auto& desc : events )
	{
		ss << std::endl << "\t{" << std::endl;
		ss << "\t\t\"type\": \"" << desc.type << "\"," << std::endl;
		if ( desc.fd != -1 )
			ss << "\t\t\"fd\": " << desc.fd << "," << std::endl;
		if ( !desc.fd_err.empty( ) )
			ss
				<< "\t\t\"error\": \"" << desc.fd_err << "\","
				<< std::endl;
		ss
			<< "\t\t\"active\": " << json_bool( desc.active )
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
}

void dispatcher::_make_dummy_event( )
{
	::pipe( pimpl_->dummy_event.pipes );

	// Pipe
	::uv_pipe_init( &pimpl_->uv_loop, &pimpl_->dummy_event.uv_pipe, 0 );
	::uv_pipe_open(
		&pimpl_->dummy_event.uv_pipe,
		pimpl_->dummy_event.pipes[ 0 ] );

	::uv_async_cb async_callback = [ ]( ::uv_async_t* async )
	{
		auto self = reinterpret_cast< dispatcher* >( async->data );

		auto task = self->pimpl_->task_fetcher_( );
		if ( !task )
			return;

		task.task( );

		// Retry with another task if its available. We could do it
		// in-place here, but instead we'll asynchronously do it,
		// meaning we'll schedule this function to be called asap, but
		// giving libuv the possibility to perform other I/O inbetween.
		::uv_async_send( async );
	};

	// Async event for triggering tasks to be performed on the I/O thread
	::uv_async_init( &pimpl_->uv_loop, &pimpl_->uv_async, async_callback );
	pimpl_->uv_async.data = this;
}

void dispatcher::_cleanup_dummy_event( )
{
	if ( true ) // TODO: If ever started
	{
		::uv_close(
			reinterpret_cast< uv_handle_t* >(
				&pimpl_->dummy_event.uv_pipe ),
			nullptr );
		::uv_close(
			reinterpret_cast< uv_handle_t* >( &pimpl_->uv_async ),
			nullptr );

		::close( pimpl_->dummy_event.pipes[ 0 ] );
		::close( pimpl_->dummy_event.pipes[ 1 ] );
	}
}

void dispatcher::start_blocking( )
{
// TODO: Implement error translation
//uv_strerror(int) and uv_err_name(int)

	::uv_run( &pimpl_->uv_loop, UV_RUN_DEFAULT );

	_cleanup_dummy_event( );

	std::cout << dump_events_json( ) << std::endl;

	::uv_loop_close( &pimpl_->uv_loop );

	termination_done( dispatcher_exit::normal );
}

void dispatcher::start( )
{
	auto self = shared_from_this( );

	auto runner = [ self ]( )
	{
		self->start_blocking( );
	};

	pimpl_->thread = q::run( pimpl_->name, pimpl_->user_queue, runner );
}

void dispatcher::notify( )
{
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
	}, pimpl_->user_queue );
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
	};

	context* ctx = new context {
		self,
		try_connect,
		dest,
		ECONNREFUSED,
		connect_callback
	};

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

		ctx->resolve_ = resolve;
		ctx->reject_ = reject;

		ctx->dest->socket_pimpl->connect_.data = ctx;

		try_connect( ctx );
	} );
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
	} );
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
	} );
}

void dispatcher::attach_event( event* event )
{
	event->attach( shared_from_this( ) );
}

void dispatcher::do_terminate( dispatcher_termination termination_method )
{
	termination dns_termination =
		termination_method == dispatcher_termination::graceful
		? termination::linger
		: termination::annihilate;

	pimpl_->dns_context_->dispatcher( )->terminate( dns_termination );

	::uv_stop( &pimpl_->uv_loop );
	::uv_async_send( &pimpl_->uv_async );
}

q::expect< > dispatcher::await_termination( )
{
	pimpl_->dns_context_->dispatcher( )->await_termination( );
	return pimpl_->thread->await_termination( );
}

} } // namespace io, namespace q
