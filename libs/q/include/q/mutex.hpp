/*
 * Copyright 2013 Gustaf Räntilä
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

#ifndef LIBQ_MUTEX_HPP
#define LIBQ_MUTEX_HPP

#include <q/types.hpp>
#include <q/type_traits.hpp>
#include <mutex>

namespace q {

// #define Q_UNIQUE_LOCK LIBQ_FILE LIBQ_LINE LIBQ_FUNCTION
// #define Q_MUTEX LIBQ_FILE LIBQ_LINE LIBQ_FUNCTION

/*
#define -DQ_DEADLOCK_DETECTION
#define -DQ_NO_DEADLOCK_DETECTION_REPORT
#define -DQ_FULL_LOCK_ANALYSIS
*/

namespace detail {

//class deadlock_
//singleton( )
//std::mutex deadlock

} // namespace detail

class representation
{
public:
	representation( const char* name = nullptr )
	: location_( )
	, name_( name )
	{ }

	explicit representation( std::string&& name )
	: location_( )
	, name_owner_( new std::string( std::move( name ) ) )
	, name_( name_owner_->c_str( ) )
	{ }

	representation( macro_location&& location,
	                const char* name = nullptr )
	: location_( location )
	, name_( name )
	{ }

	explicit representation( macro_location&& location,
	                         std::string&& name )
	: location_( location )
	, name_owner_( new std::string( std::move( name ) ) )
	, name_( name_owner_->c_str( ) )
	{ }

	const macro_location& location( ) const
	{
		return location_;
	}

	const char* name( ) const {
		if ( !name_ )
			return "";
		return name_;
	}

private:
	macro_location location_;
	std::unique_ptr< std::string > name_owner_;
	const char* name_;
};

class basic_mutex
{
public:
	virtual ~basic_mutex( )
	{ }

	virtual bool try_lock( ) = 0;
	virtual void lock( ) = 0;
	virtual void unlock( ) = 0;

protected:
	basic_mutex( )
	{ }
};

class mutex
: public basic_mutex
, public std::mutex
, public representation
{
public:
	typedef std::mutex std_type;

	mutex( const char* name = nullptr )
	: representation( name )
	{ }

	explicit mutex( std::string&& name )
	: representation( std::move( name ) )
	{ }

	mutex( macro_location location,
	       const char* name = nullptr )
	: representation( std::move( location ), name )
	{ }

	explicit mutex( macro_location location,
	                std::string&& name )
	: representation( std::move( location ), std::move( name ) )
	{ }

	bool try_lock( ) override
	{
		return std_type::try_lock( );
	}

	void lock( ) override
	{
		std_type::lock( );
	}

	void unlock( ) override
	{
		std_type::unlock( );
	}
};

class recursive_mutex
: public basic_mutex
, public std::recursive_mutex
, public representation
{
public:
	typedef std::recursive_mutex std_type;

	recursive_mutex( const char* name = nullptr )
	: representation( name )
	{ }

	explicit recursive_mutex( std::string&& name )
	: representation( std::move( name ) )
	{ }

	recursive_mutex( macro_location location,
	                 const char* name = nullptr )
	: representation( std::move( location ), name )
	{ }

	explicit recursive_mutex( macro_location location,
	                          std::string&& name )
	: representation( std::move( location ), std::move( name ) )
	{ }

	bool try_lock( ) override
	{
		return std_type::try_lock( );
	}

	void lock( ) override
	{
		std_type::lock( );
	}

	void unlock( ) override
	{
		std_type::unlock( );
	}
};

template< typename T >
struct std_mutex
{
	typedef T type;
};

template< >
struct std_mutex< mutex >
{
	typedef mutex::std_type type;
};

template< >
struct std_mutex< recursive_mutex >
{
	typedef recursive_mutex::std_type type;
};

#define Q_AUTO_UNIQUE_LOCK( ... ) \
	::q::unique_lock< \
		decltype( ::q::detail::identity_fn_noref( \
			LIBQ_FIRST( __VA_ARGS__ ) ) ) \
	> lock_unnamed_ ## LIBQ_LINE ( __VA_ARGS__ )

#define Q_UNIQUE_LOCK( ... ) \
	::q::unique_lock< \
		decltype( ::q::detail::identity_fn_noref( \
			LIBQ_FIRST( __VA_ARGS__ ) ) ) \
	>( __VA_ARGS__ )

#define Q_AUTO_UNIQUE_UNLOCK( ... ) \
	::q::unique_unlock< \
		decltype( ::q::detail::identity_fn_noref( \
			LIBQ_FIRST( __VA_ARGS__ ) ) ) \
	> unlock_unnamed_ ## LIBQ_LINE ( __VA_ARGS__ )

#define Q_UNIQUE_UNLOCK( ... ) \
	::q::unique_unlock< \
		decltype( ::q::detail::identity_fn_noref( \
			LIBQ_FIRST( __VA_ARGS__ ) ) ) \
	>( __VA_ARGS__ )

template< class Mutex >
class unique_lock
: public std::unique_lock< typename std_mutex< Mutex >::type >
, public representation
{
public:
	typedef std::unique_lock< typename std_mutex< Mutex >::type > std_type;

	unique_lock( Mutex& mutex, const char* name = nullptr )
	: std_type( mutex )
	, representation( name )
	{ }

	explicit unique_lock( Mutex& mutex, std::string&& name )
	: std_type( mutex )
	, representation( std::move( name ) )
	{ }

	unique_lock( Mutex& mutex,
	             macro_location location,
	             const char* name = nullptr )
	: std_type( mutex )
	, representation( std::move( location ), name )
	{ }

	explicit unique_lock( Mutex& mutex,
	                      macro_location location,
	                      std::string&& name )
	: std_type( mutex )
	, representation( std::move( location ), std::move( name ) )
	{ }
};

template< class Lock >
class unique_unlock
: public representation
{
public:
	unique_unlock( Lock& lock, const char* name = nullptr )
	: representation( name )
	, lock_( lock )
	{
		lock_.unlock( );
	}

	explicit unique_unlock( Lock& lock, std::string&& name )
	: representation( std::move( name ) )
	, lock_( lock )
	{
		lock_.unlock( );
	}

	unique_unlock( Lock& lock,
	               macro_location location,
	               const char* name = nullptr )
	: representation( std::move( location ), name )
	, lock_( lock )
	{
		lock_.unlock( );
	}

	explicit unique_unlock( Lock& lock,
	                        macro_location location,
	                        std::string&& name )
	: representation( std::move( location ), std::move( name ) )
	, lock_( lock )
	{
		lock_.unlock( );
	}

	~unique_unlock( )
	{
		lock_.lock( );
	}

private:
	Lock& lock_;
};

} // namespace q

#endif // LIBQ_MUTEX_HPP
