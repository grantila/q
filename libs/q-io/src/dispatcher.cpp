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

#include <q/log.hpp>
#include <q/queue.hpp>
#include <q/promise.hpp>
#include <q/lib.hpp>
#include <q/thread.hpp>
#include <q/scope.hpp>

#include <event2/event.h>
#include <event2/thread.h>

#include <condition_variable>

#include <unistd.h>

namespace q { namespace io {

namespace {

void qio_event2_log( int severity, const char* msg )
{
	q::log_level level = q::log_level::warning;

	switch ( severity )
	{
		case EVENT_LOG_DEBUG:
			level = q::log_level::debug;
			break;
		case EVENT_LOG_MSG:
			level = q::log_level::info;
			break;
		case EVENT_LOG_WARN:
			level = q::log_level::warning;
			break;
		case EVENT_LOG_ERR:
			level = q::log_level::error;
			break;
		default:
			// TODO: Extra warning logging here
			break;
	}

	typedef std::underlying_type< q::log_level >::type log_type;
	std::cout
		<< "TODO: deprecate this logging (type "
		<< static_cast< log_type >( level )
		<< "): "
		<< msg
		<< std::endl;

	// We need to ensure logging is done asynchronously, as any potential
	// libevent2 usage within this scope is illegal, such as if someone
	// would sent the log over network using libevent2...
	// TODO: Send the log over a channel< something >
	/*
	q::with( std::string( msg ) )
	.then( [ ]( std::string&& msg )
	{
		// TODO: Perform real logging.
		// Q_LOG( level ) << msg;
	} );
	*/
}

void qio_event2_fatal( int err )
{
	LIBQ_ABORT_WITH_MSG_AND_DATA( "fatal libevent2 error: ", int( err ) );
}

struct ev_lock
{
	bool recursive;

	typedef std::unique_lock< std::mutex > lock_type;
	std::unique_ptr< lock_type > lock_;
	std::unique_ptr< q::mutex > mutex_;

	typedef std::unique_lock< std::recursive_mutex > recursive_lock_type;
	std::unique_ptr< recursive_lock_type > recursive_lock_;
	std::unique_ptr< q::recursive_mutex > recursive_mutex_;
};

void* lock_alloc( unsigned locktype )
{
	auto lock = new ev_lock;

	lock->recursive = false;

	switch ( locktype )
	{
		case 0:
			lock->mutex_ = q::make_unique< q::mutex >(
				Q_HERE, "libevent2" );
			break;
		case EVTHREAD_LOCKTYPE_RECURSIVE:
			lock->recursive_mutex_ =
				q::make_unique< q::recursive_mutex >(
					Q_HERE, "libevent2" );
			lock->recursive = true;
			break;
		default:
			// TODO: Add warning logging
			delete lock;
			return nullptr;
	}

	return lock;
}

void lock_free( void* _lock, unsigned locktype )
{
	auto lock = reinterpret_cast< ev_lock* >( _lock );

	switch ( locktype )
	{
		case 0:
		case EVTHREAD_LOCKTYPE_RECURSIVE:
			delete lock;
			break;
		default:
			// TODO: Add warning logging, this is fatal
			return;
	}
}

int lock_lock( unsigned mode, void* _lock )
{
	auto lock = reinterpret_cast< ev_lock* >( _lock );

	if ( lock->recursive )
	{
		typedef ev_lock::recursive_lock_type lock_type;

		if ( mode == EVTHREAD_TRY )
		{
			lock->recursive_lock_ = q::make_unique< lock_type >(
				*lock->recursive_mutex_, std::try_to_lock_t( ) );

			return lock->recursive_lock_->owns_lock( ) ? 0 : 1;
		}
		else
		{
			lock->recursive_lock_ = q::make_unique< lock_type >(
				*lock->recursive_mutex_ );

			return 0;
		}
	}
	else
	{
		typedef ev_lock::lock_type lock_type;

		if ( mode == EVTHREAD_TRY )
		{
			lock->lock_ = q::make_unique< lock_type >(
				*lock->mutex_, std::try_to_lock_t( ) );

			return lock->lock_->owns_lock( ) ? 0 : 1;
		}
		else
		{
			lock->lock_ = q::make_unique< lock_type >(
				*lock->mutex_ );

			return 0;
		}
	}
}

int lock_unlock( unsigned mode, void* _lock )
{
	auto lock = reinterpret_cast< ev_lock* >( _lock );

	if ( lock->recursive )
	{
		lock->recursive_lock_.reset( );
	}
	else
	{
		lock->lock_.reset( );
	}

	return 0;
}

void* condition_alloc( unsigned condtype )
{
	auto cond = new std::condition_variable( );

	return reinterpret_cast< void* >( cond );
}

void condition_free( void* ptr )
{
	auto cond = reinterpret_cast< std::condition_variable* >( ptr );

	delete cond;
}

int condition_signal( void* _cond, int broadcast )
{
	auto cond = reinterpret_cast< std::condition_variable* >( _cond );

	if ( broadcast == 1 )
		cond->notify_all( );
	else
		cond->notify_one( );

	return 0;
}

int condition_wait( void* _cond, void* _lock, const struct timeval* timeout )
{
	auto cond = reinterpret_cast< std::condition_variable* >( _cond );
	auto lock = reinterpret_cast< ev_lock* >( _lock );

	if ( lock->recursive )
		// Can't conditionally wait on a recursive mutex
		return -1;

	if ( timeout )
	{
		auto time_point = clock::from_timeval( *timeout );

		return cond->wait_until( *lock->lock_, time_point )
			== std::cv_status::timeout
			? 1 : 0;
	}
	else
	{
		cond->wait( *lock->lock_ );
		return 0;
	}
}

} // anonymous namespace

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
	pimpl_->dummy_event.ev = nullptr;
	pimpl_->dummy_event.pipes[ 0 ] = pimpl_->dummy_event.pipes[ 1 ] = -1;

	::event_set_log_callback( &qio_event2_log );

#if QIO_UNDERLYING_DEBUG_LOGGING
	::event_enable_debug_logging( EVENT_DBG_ALL );
#endif

	::event_set_fatal_callback( &qio_event2_fatal );

	evthread_lock_callbacks lock_callbacks = {
		EVTHREAD_LOCK_API_VERSION,
		EVTHREAD_LOCKTYPE_RECURSIVE,
		lock_alloc,
		lock_free,
		lock_lock,
		lock_unlock
	};

	::evthread_set_lock_callbacks( &lock_callbacks );

	evthread_condition_callbacks condition_callbacks = {
		EVTHREAD_CONDITION_API_VERSION,
		condition_alloc,
		condition_free,
		condition_signal,
		condition_wait
	};

	::evthread_set_condition_callbacks( &condition_callbacks );

	event_config* cfg = ::event_config_new( );
	auto config_scope = q::make_scoped_function( [ cfg ]( )
	{
		::event_config_free( cfg );
	});

	::event_config_set_num_cpus_hint( cfg, q::hard_cores( ) );

	pimpl_->event_base = ::event_base_new_with_config( cfg );
}

dispatcher::~dispatcher( )
{
	if ( pimpl_->dummy_event.ev )
	{
		_cleanup_dummy_event( );
	}

	if ( pimpl_->event_base )
	{
		event_base_free( pimpl_->event_base );
		pimpl_->event_base = nullptr;
	}
}

std::string dispatcher::backend_method( ) const
{
	std::string method;
	method.reserve( 40 );
	method += "libevent2 using ";
	method += event_base_get_method( pimpl_->event_base );

	auto features = event_base_get_features( pimpl_->event_base );
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

	return std::move( method );
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

	event_base_event_iterator_fn fn = [ &s ]( const struct ::event* e )
	-> iteration
	{
		s += "";

		return iteration::$continue;
	};

	q::ignore_result( event_base_foreach_event( pimpl_->event_base,
	                                            &event_base_event_iterator,
	                                            &fn ) );

	return s;
}

void dispatcher::_make_dummy_event( )
{
	// This is incredibly stupid, but libevent seems not to be able to
	// function without an event being added to the event_base before it is
	// started with event_base_loop(). Also, this dummy event seems to
	// require a callback function, and must have some timeout.
	// TODO: Figure out why this is needed and get rid of it (if possible)!

	auto fn = [ ]( evutil_socket_t fd, short events, void* arg ) { };

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
}

void dispatcher::_cleanup_dummy_event( )
{
	if ( pimpl_->dummy_event.ev )
	{
		::event_del( pimpl_->dummy_event.ev );
		::close( pimpl_->dummy_event.pipes[ 0 ] );
		::close( pimpl_->dummy_event.pipes[ 1 ] );
	}
}

void dispatcher::start_blocking( )
{
	_make_dummy_event( );

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
}

void dispatcher::start( )
{
	auto self = shared_from_this( );

	auto runner = std::bind( &dispatcher::start_blocking, self );

	pimpl_->thread = q::run( pimpl_->name, pimpl_->user_queue, runner );
}

void dispatcher::notify( )
{
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

q::promise< std::tuple< resolver_response > >
dispatcher::lookup( const std::string& name )
{
	auto _resolver = q::make_shared< resolver >( shared_from_this( ) );
	return _resolver->lookup(
		pimpl_->user_queue, name, resolver::resolve_flags::normal );
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
		evutil_socket_t fd;

		~destination( )
		{
			if ( fd != -1 )
				EVUTIL_CLOSESOCKET( fd );
		}
	};

	auto dest = std::make_shared< destination >( );
	dest->addresses = std::move( addr );
	dest->pos_ipv4 = 0;
	dest->pos_ipv6 = 0;
	dest->port = port;
	dest->fd = -1;

	auto self = shared_from_this( );

	auto has_more_addresses = [ dest ]( )
	{
		return ( dest->pos_ipv4 < dest->addresses.ipv4.size( ) ) or
			( dest->pos_ipv6 < dest->addresses.ipv6.size( ) );
	};

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

void dispatcher::do_terminate( dispatcher_termination termination )
{
	int ret;

	if ( termination == dispatcher_termination::graceful )
		ret = event_base_loopexit( pimpl_->event_base, nullptr );

	else if ( termination == dispatcher_termination::immediate )
		ret = event_base_loopbreak( pimpl_->event_base );

	else
		ret = 0; // TODO: Throw exception, but this is not currently
		         // very nice to do with async_termination.

	if ( ret != 0 )
		; // TODO: Throw something, but read above.
}

q::expect< > dispatcher::await_termination( )
{
	return pimpl_->thread->await_termination( );
}

} } // namespace io, namespace q
