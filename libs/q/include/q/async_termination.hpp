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

#ifndef LIBQ_ASYNC_TERMINATION_HPP
#define LIBQ_ASYNC_TERMINATION_HPP

#include <q/promise.hpp>

#include <type_traits>

namespace q {

namespace detail {

// Synchronous termination

template< typename Parameters >
class sync_termination_interface;

template< typename... Args >
class sync_termination_interface< q::arguments< Args... > >
{
	typedef sync_termination_interface< arguments< Args... > > this_type;

public:
	template< typename... FnArgs >
	typename std::enable_if<
		::q::is_argument_same_or_convertible_t<
			q::arguments< FnArgs... >,
			q::arguments< Args... >
		>::value
	>::type
	terminate( FnArgs&&... args )
	{
		::q::call_with_args(
			&this_type::do_terminate,
			this,
			std::forward< FnArgs >( args )... );
	}

protected:
	virtual void do_terminate( Args... ) = 0;
};

// Asynchronous termination

template< typename Parameters, typename... Completion >
class async_termination_interface;

template< typename... Args, typename... Completion >
class async_termination_interface< q::arguments< Args... >, Completion... >
{
public:
	template< typename... FnArgs >
	typename std::enable_if<
		::q::is_argument_same_or_convertible_t<
			q::arguments< FnArgs... >,
			q::arguments< Args... >
		>::value,
		promise< Completion... >
	>::type
	terminate( FnArgs&&... args );

protected:
	virtual void do_terminate( Args... ) = 0;
};

template<
	typename Parameters = q::arguments< >,
	typename... Completion
>
class async_termination
: public async_termination_interface< Parameters, Completion... >
{
public:
	typedef detail::suitable_defer_t< Completion... > defer_type;

	virtual ~async_termination( ) { }

protected:
	async_termination( const queue_ptr& queue )
	: deferred_termination_( ::q::make_shared< defer_type >( queue ) )
	{ }
	async_termination( const async_termination& ) = delete;
	async_termination( async_termination&& ) = delete;

	defer_type& termination_deferer( )
	{
		return *deferred_termination_.get( );
	}

	void termination_done( std::tuple< Completion... >&& completion )
	{
		termination_deferer( ).set_value( std::move( completion ) );
	}

	void termination_done( const std::tuple< Completion... >& completion )
	{
		termination_deferer( ).set_value( completion );
	}

	template< typename... Args >
	typename std::enable_if<
		::q::is_argument_same_or_convertible_t<
			::q::arguments< Args... >,
			::q::arguments< Completion... >
		>::value
	>::type
	termination_done( Args&&... args )
	{
		termination_done( std::forward_as_tuple( args... ) );
	}

private:
	template< typename, typename... >
	friend class async_termination_interface;

	// TODO: Convert to safe_shared_ptr
	std::shared_ptr< defer_type > deferred_termination_;
};

template< typename... Args, typename... Completion >
template< typename... FnArgs >
typename std::enable_if<
	::q::is_argument_same_or_convertible_t<
		q::arguments< FnArgs... >,
		q::arguments< Args... >
	>::value,
	promise< Completion... >
>::type
async_termination_interface< q::arguments< Args... >, Completion... >::
terminate( FnArgs&&... args )
{
	typedef async_termination<
		q::arguments< Args...>, Completion...
	> subclass;

	auto& at = static_cast< subclass& >( *this );

	try
	{
		::q::call_with_args(
			&subclass::do_terminate,
			this,
			std::forward< FnArgs >( args )... );
	}
	catch ( ... )
	{
		// Now we're in a very bad state. It is semantically wrong to
		// at all end up here - it means the do_terminate threw an
		// exception, and we have no idea if termination_done( ) will
		// ever be called or not.
		// TODO: Potentially log this as a critical error
		at.deferred_termination_->set_current_exception( );
	}

	return at.deferred_termination_->get_promise( );
}

} // namespace detail

/**
 * async_termination provides an interface for asynchronously terminating an
 * object. This is useful for objects which needs non-direct de-initialization
 * mechanisms, such as waiting for I/O, joining threads, etc.
 *
 * The template argument @c Parameters defines the parameters the terminate()
 * function will require. This is supposed to be either one specific type, or
 * a @c q::arguments type, in which case the @c q::arguments' inner types are
 * expanded to the terminate() function.
 *
 * The template argument @c Completion defines the type returned by the
 * returning promise of the terminate() function. This must be a tuple of some
 * kind.
 */
template<
	typename Parameters = q::arguments< >,
	typename... Completion
>
class async_termination
: public detail::async_termination< Parameters, Completion... >
{
public:
	virtual ~async_termination( ) { }

protected:
	async_termination( ) = delete;
	async_termination( const async_termination& ) = delete;
	async_termination( async_termination&& ) = delete;

	async_termination( const queue_ptr& queue )
	: detail::async_termination< Parameters, Completion... >( queue )
	{ }
};

template<
	typename Parameters
>
class async_termination< Parameters >
: public detail::async_termination< Parameters >
{
public:
	virtual ~async_termination( ) { }

protected:
	async_termination( ) = delete;
	async_termination( const async_termination& ) = delete;
	async_termination( async_termination&& ) = delete;

	async_termination( const queue_ptr& queue )
	: detail::async_termination< Parameters >( queue )
	{ }
};

/**
 * Synchronous version of the async_termination, where the terminate( ) returns
 * when the termination is actually complete, instead of returning a promise.
 */
template<
	typename Parameters
>
class sync_termination
: public detail::sync_termination_interface< Parameters >
{
public:
	virtual ~sync_termination( ) { }

protected:
	sync_termination( ) = default;
	sync_termination( const sync_termination& ) = delete;
	sync_termination( sync_termination&& ) = delete;
};

} // namespace q

#endif // LIBQ_ASYNC_TERMINATION_HPP
