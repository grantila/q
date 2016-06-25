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
#include <q-io/socket.hpp>
#include <q-io/server_socket.hpp>

#include "internals.hpp"
#include "socket_helpers.hpp"

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
#ifdef QIO_USE_LIBEVENT
	pimpl_->dummy_event.ev = nullptr;
#endif
	pimpl_->dummy_event.pipes[ 0 ] = pimpl_->dummy_event.pipes[ 1 ] = -1;

#ifndef QIO_USE_LIBEVENT_DNS
	pimpl_->dns_context_ = q::make_execution_context< q::threadpool >(
		"q-io dns worker", user_queue, 6 );
	pimpl_->dns_queue_ = pimpl_->dns_context_->queue( );
#endif

#ifdef QIO_USE_LIBEVENT
	event_config* cfg = ::event_config_new( );
	auto config_scope = q::make_scoped_function( [ cfg ]( )
	{
		::event_config_free( cfg );
	});

	::event_config_set_num_cpus_hint( cfg, q::hard_cores( ) );

	pimpl_->event_base = ::event_base_new_with_config( cfg );
#else
	// libuv
	::uv_loop_init( &pimpl_->uv_loop );
#endif
}

dispatcher::~dispatcher( )
{
	if ( true ) // TODO: has ever started
	{
		_cleanup_dummy_event( );
	}

#ifdef QIO_USE_LIBEVENT
	if ( pimpl_->event_base )
	{
		::event_base_free( pimpl_->event_base );
		pimpl_->event_base = nullptr;
	}
#endif
}

std::string dispatcher::backend_method( ) const
{
	std::string method;
// TODO: Remove this, or make libuv-compatible
#ifdef QIO_USE_LIBEVENT
	method.reserve( 40 );
	method += "libevent2 using ";
	method += ::event_base_get_method( pimpl_->event_base );

	auto features = ::event_base_get_features( pimpl_->event_base );
	bool et = ( features & EV_FEATURE_ET ) == EV_FEATURE_ET;
	bool o1 = ( features & EV_FEATURE_O1 ) == EV_FEATURE_O1;
	if ( et or o1 )
	{
		method += " (";
		if ( et )
			method += "ET";
		if ( o1 )
		{
			if ( et )
				method += ", ";
			method += "O(1)";
		}
		method += ")";
	}
#endif

	return method;
}

std::string dispatcher::dump_events( ) const
{
	std::string s;

// TODO: Remove this, or make libuv-compatible
#ifdef QIO_USE_LIBEVENT

	event_base_event_iterator_fn fn = [ &s ]( const struct ::event* e )
	-> iteration
	{
		void* arg = ::event_get_callback_arg( e );
		auto ev_arg = reinterpret_cast< callback_arg_type* >( arg );

		ev_arg;

		s += "";

		return iteration::$continue;
	};

	q::ignore_result( ::event_base_foreach_event(
		pimpl_->event_base,
		&event_base_event_iterator,
		&fn ) );
#endif
	uv_walk_cb walker = [ ]( ::uv_handle_t* handle, void* arg )
	{
		auto handle_type_name = [ ]( ::uv_handle_type type )
		-> std::string
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

		std::cout << "EVENT" << std::endl;
		std::cout << "    Type       : " << handle_type_name( handle->type ) << std::endl;
		std::cout << "    Active     : " << ::uv_is_active( handle ) << std::endl;
		std::cout << "    Is closing : " << ::uv_is_closing( handle ) << std::endl;

		bool is_fd_backed =
			handle->type == ::uv_handle_type::UV_TCP ||
			handle->type == ::uv_handle_type::UV_NAMED_PIPE ||
			handle->type == ::uv_handle_type::UV_TTY ||
			handle->type == ::uv_handle_type::UV_UDP ||
			handle->type == ::uv_handle_type::UV_POLL;

		if ( is_fd_backed )
		{
			uv_os_fd_t fd;
			auto ret = ::uv_fileno( handle, &fd );
			if ( ret )
			{
				std::cout << "    fd error   : ";
				if ( ret == UV_EINVAL )
					std::cout << "UV_EINVAL";
				else if ( ret == UV_EBADF )
					std::cout << "UV_EBADF";
				else
					std::cout << ret;
				std::cout << std::endl;
			}
			else
				std::cout << "    fd         : " << fd << std::endl;
		}
	};
	::uv_walk( &pimpl_->uv_loop, walker, nullptr );

	return s;
}

void dispatcher::_make_dummy_event( )
{
#ifdef QIO_USE_LIBEVENT
	// This is incredibly stupid, but libevent seems not to be able to
	// function without an event being added to the event_base before it is
	// started with event_base_loop(). Also, this dummy event seems to
	// require a callback function, and must have some timeout.
	// TODO: Figure out why this is needed and get rid of it (if possible)!

	event_callback_fn fn = [ ](
		evutil_socket_t fd, short events, void* arg
	)
	{ };

	::pipe( pimpl_->dummy_event.pipes );

	pimpl_->dummy_event.ev = ::event_new(
		pimpl_->event_base,
		pimpl_->dummy_event.pipes[ 0 ],
		EV_READ | EV_PERSIST,
		fn,
		nullptr );

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 1;
	::event_add( pimpl_->dummy_event.ev, &timeout );
#else
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

		task( );

		// Retry with another task if its available. We could do it
		// in-place here, but instead we'll asynchronously do it,
		// meaning we'll schedule this function to be called asap, but
		// giving libuv the possibility to perform other I/O inbetween.
		::uv_async_send( async );
	};

	// Async event for triggering tasks to be performed on the I/O thread
	::uv_async_init( &pimpl_->uv_loop, &pimpl_->uv_async, async_callback );
	pimpl_->uv_async.data = this;
#endif
}

void dispatcher::_cleanup_dummy_event( )
{
	if ( true ) // TODO: If ever started
	{
#ifdef QIO_USE_LIBEVENT
		::event_del( pimpl_->dummy_event.ev );
#else
		::uv_close(
			reinterpret_cast< uv_handle_t* >(
				&pimpl_->dummy_event.uv_pipe ),
			nullptr );
		::uv_close(
			reinterpret_cast< uv_handle_t* >( &pimpl_->uv_async ),
			nullptr );
#endif
		::close( pimpl_->dummy_event.pipes[ 0 ] );
		::close( pimpl_->dummy_event.pipes[ 1 ] );
	}
}

void dispatcher::start_blocking( )
{
// TODO: Implement error translation
//uv_strerror(int) and uv_err_name(int)

	_make_dummy_event( );

#ifdef QIO_USE_LIBEVENT
	int flags = EVLOOP_NO_EXIT_ON_EMPTY;
	int ret = event_base_loop( pimpl_->event_base, flags );

	bool got_exit = event_base_got_exit( pimpl_->event_base );
	bool got_break = event_base_got_break( pimpl_->event_base );

	dispatcher_exit _exit = dispatcher_exit::normal;

	if ( ret != 0 )
		_exit = dispatcher_exit::failed;
	else
	{
		if ( got_break )
			_exit = dispatcher_exit::forced;
		else if ( got_exit )
			_exit = dispatcher_exit::exited;
	}

	const std::tuple< dispatcher_exit > _ret( _exit );

	termination_done( _exit );
#else
	::uv_run( &pimpl_->uv_loop, UV_RUN_DEFAULT );

	::uv_loop_close( &pimpl_->uv_loop );

	termination_done( dispatcher_exit::normal );
#endif
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
#ifdef QIO_USE_LIBEVENT
	auto self = shared_from_this( );

	while ( true )
	{
		auto task = pimpl_->task_fetcher_( );
		if ( !task )
			return;

		pimpl_->tasks_.push( task );

		struct X
		{
			dispatcher_ptr self;
			q::task task;
		};

		X* x = new X { self, task };

		auto fn = [ ]( evutil_socket_t fd, short events, void* arg )
		-> void
		{
			std::cout << "Inner fun" << std::endl;
			auto x = reinterpret_cast< X* >( arg );
			q::task task = std::move( x->task );
			delete x;

			task( );
		};

		::event* ev = ::event_new( pimpl_->event_base, -1, 0, fn, x );

		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 1;
		::event_add( ev, &timeout );

		// ::event_active( ev, 0, 0 );
	};
#else
	::uv_async_send( &pimpl_->uv_async );
#endif
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
		auto timer = std::make_shared< timer_task >( );

		self->attach_event( timer );

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

q::promise< std::tuple< resolver_response > >
dispatcher::lookup( const std::string& name )
{
#ifdef QIO_USE_LIBEVENT_DNS
	auto _resolver = q::make_shared< resolver >( shared_from_this( ) );
	return _resolver->lookup(
		pimpl_->user_queue, name, resolver::resolve_flags::normal );
#else
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
#endif
}

// Tries to connect to the first IP address, then the next, etc, until all
// addresses are tried.
q::promise< std::tuple< socket_ptr > > dispatcher::connect_to(
	ip_addresses&& addr, std::uint16_t port )
{
	struct destination
	{
		ip_addresses addresses;
		std::size_t pos_ipv4;
		std::size_t pos_ipv6;
		std::uint16_t port;

		ip_addresses::iterator cur_addr;
		ip_addresses::iterator end_addr;
		std::shared_ptr< socket::pimpl > socket_pimpl;
	};

	auto dest = std::make_shared< destination >( );
	dest->addresses = std::move( addr );
	dest->pos_ipv4 = 0;
	dest->pos_ipv6 = 0;
	dest->port = port;

	dest->cur_addr = dest->addresses.begin( port );
	dest->end_addr = dest->addresses.end( port );
	dest->socket_pimpl = std::make_shared< socket::pimpl >( );

	auto self = shared_from_this( );

	return q::make_promise( get_queue( ),
		[ self, dest ](
			q::resolver< socket_ptr > resolve,
			q::rejecter< socket_ptr > reject
		)
	{
		struct context
		{
			dispatcher_ptr dispatcher;
			q::resolver< socket_ptr > resolve_;
			q::rejecter< socket_ptr > reject_;
			std::function< void( context* ) > try_connect;
			std::shared_ptr< destination > dest;
			std::shared_ptr< sockaddr > last_address;
			int last_error;
			uv_connect_cb connect_callback;

			void resolve( socket_ptr&& socket )
			{
				resolve_( std::move( socket ) );
				free( );
			}

			void reject( std::exception_ptr&& e )
			{
				reject_( std::move( e ) );
				free( );
			}

		private:
			void free( )
			{
				delete this;
			}
			~context( ) { }
		};

		::uv_tcp_init(
			&self->pimpl_->uv_loop, &dest->socket_pimpl->socket_ );

		uv_connect_cb connect_callback = [ ]( uv_connect_t* req, int status )
		{
			auto ctx = reinterpret_cast< context* >( req->data );

			if ( status == 0 )
			{
				// Success
				auto& socket_event_pimpl =
					ctx->dest->socket_pimpl;
				auto sock = ::q::io::socket::construct(
					std::move( socket_event_pimpl ) );
				sock->attach( ctx->dispatcher );

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
			auto& connect = ctx->dest->socket_pimpl->connect_;
			auto& socket = ctx->dest->socket_pimpl->socket_;

			if ( ctx->dest->cur_addr == ctx->dest->end_addr )
			{
				auto& ipv4 = ctx->dest->addresses.ipv4;
				auto& ipv6 = ctx->dest->addresses.ipv6;
				int _errno = ipv4.empty( ) && ipv6.empty( )
					? EINVAL
					: ctx->last_error;

				ctx->reject( get_exception_by_errno( _errno ) );
				return;
			}

			ctx->last_address = *ctx->dest->cur_addr++;

			::uv_tcp_connect(
				&connect,
				&socket,
				ctx->last_address.get( ),
				ctx->connect_callback
			);
		};

		context* ctx = new context {
			self,
			resolve,
			reject,
			try_connect,
			dest,
			nullptr,
			ECONNREFUSED,
			connect_callback
		};

		dest->socket_pimpl->connect_.data = ctx;

		try_connect( ctx );
	} );
}

q::promise< std::tuple< server_socket_ptr > >
dispatcher::listen( std::uint16_t port, ip_addresses&& bind_to )
{
#ifndef QIO_USE_LIBEVENT
	auto self = shared_from_this( );
	auto server = server_socket::construct( port, std::move( bind_to ) );

	return q::make_promise( get_queue( ), [ self, server ]( )
	{
		server->attach( self );

		return server;
	} );
#else
	auto dest = ip_addresses( "127.0.0.1" );

	struct sockaddr_in addr;
	dest.ipv4[ 0 ].populate( addr, port );

	auto fd = create_socket( PF_INET );

	auto ret = ::bind( fd, reinterpret_cast< const sockaddr* >( &addr ), sizeof addr );

	if ( ret == -1 )
	{
		auto errno_ = errno;
		EVUTIL_CLOSESOCKET( fd );
		throw_by_errno( errno_ );
	}

	ret = ::listen( fd, 64 ); // TODO: Reconsider backlog

	if ( ret == -1 )
	{
		auto errno_ = errno;
		EVUTIL_CLOSESOCKET( fd );
		throw_by_errno( errno_ );
	}

	auto server_socket = server_socket::construct( fd );

	server_socket->attach( shared_from_this( ) );

	return server_socket;
#endif
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

#ifdef QIO_USE_LIBEVENT
	int ret;

	if ( termination_method == dispatcher_termination::graceful )
		ret = event_base_loopexit( pimpl_->event_base, nullptr );

	else if ( termination_method == dispatcher_termination::immediate )
		ret = event_base_loopbreak( pimpl_->event_base );

	else
		ret = 0; // TODO: Throw exception, but this is not currently
		         // very nice to do with async_termination.

	if ( ret != 0 )
		; // TODO: Throw something, but read above.
#else
	::uv_stop( &pimpl_->uv_loop );
	::uv_async_send( &pimpl_->uv_async );
#endif
}

q::expect< > dispatcher::await_termination( )
{
	pimpl_->dns_context_->dispatcher( )->await_termination( );
	return pimpl_->thread->await_termination( );
}

} } // namespace io, namespace q
