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
#include <q-io/timeout_event.hpp>
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
, pimpl_( new pimpl )
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
	if ( true /* TODO: has ever started */ )
	{
		_cleanup_dummy_event( );
	}

#ifdef QIO_USE_LIBEVENT
	if ( pimpl_->event_base )
	{
		::event_base_free( pimpl_->event_base );
		pimpl_->event_base = nullptr;
	}
#else
	::uv_loop_close( &pimpl_->uv_loop );
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

namespace {

enum class iteration
{
	$continue,
	$break
};

typedef std::function<
	iteration( const struct ::event* )
> event_base_event_iterator_fn;

int event_base_event_iterator( const struct ::event_base*,
                               const struct ::event* ev,
                               void* user )
{
	auto& fn = *reinterpret_cast< event_base_event_iterator_fn* >( user );
	switch ( fn( ev ) )
	{
		case iteration::$continue:
			return 0;
		default:
			return 1;
	}
}

} // anonymous namespace

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
	if ( true /* TODO: If ever started */ )
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

	auto runner = std::bind( &dispatcher::start_blocking, self );

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
		auto task = [ fn ]( ) mutable
		{
			fn( q::fulfill< void >( ) );
		};

		self->attach_event( timeout_task::construct( task, dur_ ) );
	};

	return q::async_task( runner );
}

template< typename T >
T identity( T&& t ) { return t; }

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
			Q_THROW( dns_lookup_error( ) /* << err */ );
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
#ifdef QIO_USE_LIBEVENT
		evutil_socket_t fd;

		~destination( )
		{
			if ( fd != -1 )
				EVUTIL_CLOSESOCKET( fd );
		}
#else
		std::unique_ptr< socket_event::pimpl > socket_event_pimpl;
#endif
	};

	auto dest = std::make_shared< destination >( );
	dest->addresses = std::move( addr );
	dest->pos_ipv4 = 0;
	dest->pos_ipv6 = 0;
	dest->port = port;
#ifdef QIO_USE_LIBEVENT
	dest->fd = -1;
#else
	dest->socket_event_pimpl = q::make_unique< socket_event::pimpl >( );

	::uv_tcp_init( &pimpl_->uv_loop, &dest->socket_event_pimpl->socket );
#endif

	auto self = shared_from_this( );

	auto has_more_addresses = [ dest ]( )
	{
		return ( dest->pos_ipv4 < dest->addresses.ipv4.size( ) ) or
			( dest->pos_ipv6 < dest->addresses.ipv6.size( ) );
	};

#ifndef QIO_USE_LIBEVENT
	// Returns the next sockaddr (or nullptr)
	auto get_next_address = [ self, dest ]( ) -> std::unique_ptr< sockaddr >
	{
		if ( dest->pos_ipv4 < dest->addresses.ipv4.size( ) )
		{
			const ipv4_address& ipv4_address =
				dest->addresses.ipv4[ dest->pos_ipv4++ ];

			auto addr_in = q::make_unique< sockaddr_in >( );

			ipv4_address.populate( *addr_in, dest->port );

			return std::unique_ptr< sockaddr >(
				reinterpret_cast< sockaddr* >(
					addr_in.release( ) ) );
		}

		if ( dest->pos_ipv6 < dest->addresses.ipv6.size( ) )
		{
			const ipv6_address& ipv6_address =
				dest->addresses.ipv6[ dest->pos_ipv6++ ];

			auto addr_in6 = q::make_unique< sockaddr_in6 >( );

			ipv6_address.populate( *addr_in6, dest->port );

			return std::unique_ptr< sockaddr >(
				reinterpret_cast< sockaddr* >(
					addr_in6.release( ) ) );
		}

		return nullptr;
	};

	return q::make_promise_sync( pimpl_->user_queue,
		[ self, dest, get_next_address, has_more_addresses ](
			q::resolver< socket_ptr > resolve,
			q::rejecter< socket_ptr > reject
		)
	{
		struct context
		{
			dispatcher_ptr dispatcher;
			q::resolver< socket_ptr > resolve_;
			q::rejecter< socket_ptr > reject_;
			std::function< std::unique_ptr< sockaddr >( void ) >
				get_next_address;
			std::function< bool( void ) > has_more_addresses;
			std::function< void( context* ) > try_connect;
			std::shared_ptr< destination > dest;
			std::unique_ptr< sockaddr > last_address;
			int last_error;

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

		uv_connect_cb connect_callback = [ ]( uv_connect_t* req, int status )
		{
			auto ctx = reinterpret_cast< context* >( req->data );

			if ( status == 0 )
			{
				// Success
				auto& socket_event_pimpl =
					ctx->dest->socket_event_pimpl;
				auto sock = ::q::io::socket::construct(
					std::move( socket_event_pimpl ) );
				sock->attach( ctx->dispatcher );

				ctx->resolve( std::move( sock ) );
			}
			else
			{
				// Error
				ctx->last_error = -status; // TODO: Translate for win32
				ctx->try_connect( ctx );
			}
		};

		auto try_connect = [ connect_callback ]( context* ctx )
		{
			auto& connect = ctx->dest->socket_event_pimpl->connect;
			auto& socket = ctx->dest->socket_event_pimpl->socket;

			if ( !ctx->has_more_addresses( ) )
			{
				auto& ipv4 = ctx->dest->addresses.ipv4;
				auto& ipv6 = ctx->dest->addresses.ipv6;
				int _errno = ipv4.empty( ) && ipv6.empty( )
					? EINVAL
					: ctx->last_error;

				ctx->reject( get_exception_by_errno( _errno ) );
				return;
			}

			ctx->last_address = ctx->get_next_address( );

			::uv_tcp_connect(
				&connect,
				&socket,
				ctx->last_address.get( ),
				connect_callback
			);
		};

		context* ctx = new context {
			self,
			resolve,
			reject,
			get_next_address,
			has_more_addresses,
			try_connect,
			dest,
			nullptr,
			ECONNREFUSED
		};

		dest->socket_event_pimpl->connect.data = ctx;

		try_connect( ctx );
	} );
#else
	// Performs a connect against the next not-yet-tried address
	// Returns true if connect was synchronously successful (i.e. we must
	// await the result of the connect)
	auto try_next_address = [ self, dest ]( ) -> std::pair< bool, int >
	{
		if ( dest->pos_ipv4 < dest->addresses.ipv4.size( ) )
		{
			if ( dest->fd != -1 )
			{
				EVUTIL_CLOSESOCKET( dest->fd );
				dest->fd = -1;
			}

			const ipv4_address& ipv4_address =
				dest->addresses.ipv4[ dest->pos_ipv4++ ];

			struct sockaddr_in addr;

			ipv4_address.populate( addr, dest->port );

			dest->fd = create_socket( PF_INET );

			return q::io::connect( dest->fd, addr );
		}

		if ( dest->pos_ipv6 < dest->addresses.ipv6.size( ) )
		{
			if ( dest->fd != -1 )
			{
				EVUTIL_CLOSESOCKET( dest->fd );
				dest->fd = -1;
			}

			const ipv6_address& ipv6_address =
				dest->addresses.ipv6[ dest->pos_ipv6++ ];

			struct sockaddr_in6 addr;

			ipv6_address.populate( addr, dest->port );

			dest->fd = create_socket( PF_INET6 );

			return q::io::connect( dest->fd, addr );
		}

		return std::make_pair( false, EINVAL );
	};

	int last_error = EINVAL;
	bool first_success = false;
	while ( has_more_addresses( ) && !first_success )
	{
		auto ret = try_next_address( );
		first_success = ret.first;
		last_error = ret.second;
	}

	if ( !first_success )
		return q::reject< q::arguments< socket_ptr > >(
			pimpl_->user_queue,
			get_exception_by_errno( last_error ) );

	return q::make_promise_sync( pimpl_->user_queue,
		[ self, dest, try_next_address, has_more_addresses ](
			q::resolver< socket_ptr > resolve,
			q::rejecter< socket_ptr > reject
		)
	{
		struct context
		{
			dispatcher_ptr dispatcher;
			q::resolver< socket_ptr > resolve;
			q::rejecter< socket_ptr > reject;
			std::function< std::pair< bool, int >( void ) >
				try_connect;
			std::function< bool( void ) > has_more_addresses;
			std::shared_ptr< destination > dest;
			event_callback_fn cb;
		};

		auto event_cb = [ ](
			evutil_socket_t socket, short what, void* arg )
		{
			context* ctx = reinterpret_cast< context* >( arg );

			const dispatcher_ptr& dispatcher = ctx->dispatcher;
			destination& dest = *ctx->dest;

			::event_base* base = dispatcher->pimpl_->event_base;

			int err;
			socklen_t len = sizeof err;
			auto ret = ::getsockopt(
				socket, SOL_SOCKET, SO_ERROR, &err, &len );

			if ( !ret && !err )
			{
				// Connection success
				auto sock = ::q::io::socket::construct( socket );

				sock->attach( dispatcher );

				ctx->resolve( sock );

				// Remove fd from destination to avoid closing
				// it
				dest.fd = -1;

				delete ctx;

				return;
			}

			if ( ret == -1 )
				err = errno;

			int errno_ = err;

			while ( ctx->has_more_addresses( ) )
			{
				auto ret = ctx->try_connect( );
				errno_ = ret.second;

				if ( ret.first )
				{
					// Try again
					::event_base_once(
						base,
						dest.fd,
						EV_WRITE,
						ctx->cb,
						ctx,
						nullptr );
					return;
				}
			}

			ctx->reject( get_exception_by_errno( errno_ ) );

			delete ctx;
		};

		context* ctx = new context {
			self,
			resolve,
			reject,
			try_next_address,
			has_more_addresses,
			dest,
			event_cb
		};

		::event_base_once(
			self->pimpl_->event_base,
			dest->fd,
			EV_WRITE,
			ctx->cb,
			ctx,
			nullptr );
	} );
#endif
}

server_socket_ptr dispatcher::listen( std::uint16_t port )
{
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
}

void dispatcher::attach_event( const event_ptr& event )
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
#endif
}

q::expect< > dispatcher::await_termination( )
{
	pimpl_->dns_context_->dispatcher( )->await_termination( );
	return pimpl_->thread->await_termination( );
}

} } // namespace io, namespace q
