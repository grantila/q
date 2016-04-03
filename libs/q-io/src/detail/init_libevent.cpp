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

#include <q-io/clock.hpp>

#include "init_libevent.hpp"

#include <q/lib.hpp>
#include <q/log.hpp>
#include <q/thread.hpp>

#include <event2/event.h>
#include <event2/thread.h>

#include <condition_variable>

namespace q { namespace io { namespace detail {

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

void global_init_libevent( )
{
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
}

} } } // namespace detail, namespace io, namespace q
