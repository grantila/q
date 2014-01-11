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

template< typename Parameters, typename Completion >
class async_termination_interface;

template< typename... Args, typename Completion >
class async_termination_interface< q::arguments< Args... >, Completion >
{
public:
	template< typename... FnArgs >
	typename std::enable_if<
		::q::is_argument_same_or_convertible<
			q::arguments< FnArgs... >,
			q::arguments< Args... >
		>::value,
		promise< Completion >
	>::type
	terminate( FnArgs&&... args );

protected:
	virtual void do_terminate( Args... ) = 0;
};

template<
	typename Parameters = q::arguments< >,
	typename Completion = std::tuple< >
>
class async_termination
: public async_termination_interface< Parameters, Completion >
{
public:
	typedef typename ::q::tuple_arguments< Completion >
		::template apply< ::q::detail::defer >::type defer_type;

	virtual ~async_termination( ) { }

protected:
	async_termination( )
	: deferred_termination_( ::q::make_shared< defer_type >( ) )
	{ }
	async_termination( const async_termination& ) = delete;
	async_termination( async_termination&& ) = delete;

	defer_type& termination_deferer( )
	{
		return *deferred_termination_.get( );
	}

	void termination_done( Completion&& completion )
	{
		termination_deferer( ).set_value( std::move( completion ) );
	}

	void termination_done( const Completion& completion )
	{
		termination_deferer( ).set_value( completion );
	}

	template< typename... Args >
	typename std::enable_if<
		::q::is_argument_same_or_convertible<
			::q::arguments< Args... >,
			typename ::q::arguments_from_tuple< Completion >::type
		>::value
	>::type
	termination_done( Args&&... args )
	{
		termination_done( std::forward_as_tuple( args... ) );
	}

private:
	template< typename, typename >
	friend class async_termination_interface;

	// TODO: Convert to safe_shared_ptr
	std::shared_ptr< defer_type > deferred_termination_;
};

template< typename... Args, typename Completion >
template< typename... FnArgs >
typename std::enable_if<
	::q::is_argument_same_or_convertible<
		q::arguments< FnArgs... >,
		q::arguments< Args... >
	>::value,
	promise< Completion >
>::type
async_termination_interface< q::arguments< Args... >, Completion >::
terminate( FnArgs&&... args )
{
	typedef async_termination< q::arguments< Args...>, Completion > subclass;
	auto& at = static_cast< subclass& >( *this );

	::q::call_with_args(
		&async_termination_interface< q::arguments< Args... >, Completion >::do_terminate,
		this,
		std::forward< FnArgs >( args )... );

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
	typename Completion = std::tuple< >
>
class async_termination
: public detail::async_termination< Parameters, Completion >
{
public:
	virtual ~async_termination( ) { }

protected:
	async_termination( ) = default;
	async_termination( const async_termination& ) = delete;
	async_termination( async_termination&& ) = delete;
};

template<
	typename Parameters
>
class async_termination< Parameters, std::tuple< > >
: public detail::async_termination< Parameters, std::tuple< > >
{
public:
	virtual ~async_termination( ) { }

protected:
	async_termination( ) = default;
	async_termination( const async_termination& ) = delete;
	async_termination( async_termination&& ) = delete;
};

} // namespace q

#endif // LIBQ_ASYNC_TERMINATION_HPP
