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

#ifndef LIBQ_EXCEPTION_EXCEPTION_HPP
#define LIBQ_EXCEPTION_EXCEPTION_HPP

#include <q/pp.hpp>
#include <q/types.hpp>
#include <q/type_traits.hpp>
#include <q/stacktrace.hpp>

#include <exception>
#include <vector>
#include <iosfwd>
#include <sstream>
#include <system_error>
#include <functional>

#define Q_THROW( ... ) \
	throw ::q::add_exception_properties_with_stacktrace( __VA_ARGS__ )

#define Q_THROW_NO_STACKTRACE( ... ) \
	throw ::q::add_exception_properties( __VA_ARGS__ )

#define Q_MAKE_SIMPLE_EXCEPTION( Name ) \
	class Name \
	: public ::q::exception \
	{ \
		virtual const char* name( ) const noexcept override \
		{ \
			return #Name ; \
		} \
	}

#define Q_MAKE_STANDARD_EXCEPTION_OF_BASE( Name, Base ) \
	class Name \
	: public ::q::exception \
	, public Base \
	{ \
	public: \
		template< typename... Args > \
		Name( Args&&... args ) \
		: Base( std::forward< Args >( args )... ) \
		{ } \
		\
		virtual const char* name( ) const noexcept override \
		{ \
			return #Name ; \
		} \
	}

#define Q_MAKE_STANDARD_EXCEPTION( Name ) \
	Q_MAKE_STANDARD_EXCEPTION_OF_BASE( Name, ::std::Name )

/**
 * Run @c fn and ensure that it doesn't throw any exception. If it does, invoke
 * the "uncaught exception handler".
 */
#define Q_ENSURE_NOEXCEPT( fn ) \
	do \
	{ \
		try \
		{ \
			fn( ); \
		} \
		catch ( ... ) \
		{ \
			LIBQ_UNCAUGHT_EXCEPTION( std::current_exception( ) ); \
		} \
	} while ( false )

namespace q {

namespace detail {

template< typename T > class q_exception_info;

class exception_info_base
{
public:
	virtual ~exception_info_base( )
	{ }

	template< typename T >
	const q_exception_info< T >* cast( ) const
	{
		return dynamic_cast< const q_exception_info< T >* >( this );
	}

	template< typename T >
	q_exception_info< T >* cast( )
	{
		return dynamic_cast< q_exception_info< T >* >( this );
	}

	virtual std::string string( ) const = 0;

protected:
	exception_info_base( )
	{ }
};

template< typename T >
class q_exception_info
: public exception_info_base
{
public:
	q_exception_info( ) = delete;
	q_exception_info( const q_exception_info& ) = delete;

	q_exception_info( q_exception_info&& ) = default;

	q_exception_info( T&& t )
	: t_( std::move( t ) )
	{ }

	q_exception_info( const T& t )
	: t_( t )
	{ }

	const T& get( ) const
	{
		return t_;
	}

	T consume( )
	{
		return std::move( t_ );
	}

	static q_exception_info< T >* cast( exception_info_base& eib )
	{
		return eib.cast< T >( );
	}

	static const q_exception_info< T >* cast( const exception_info_base& eib )
	{
		return eib.cast< T >( );
	}

	std::string string( ) const override
	{
		std::stringstream ss;
		ss << t_;
		return ss.str( );
	}

private:
	T t_;
};

} // namespace detail

class exception
: std::exception
{
public:
	exception( );
	exception( exception&& ) = default;
	exception( const exception& ) = default;
	virtual ~exception( );

	exception& operator=( const exception& ) = default;
	exception& operator=( exception&& ) = default;

	template< typename T >
	const detail::q_exception_info< T >* get_info( ) const
	{
		auto& _infos = infos( );
		for ( const auto& info : _infos )
		{
			auto ei = detail::q_exception_info< T >::cast( *info );
			if ( ei )
				return ei;
		}
		return nullptr;
	}

	template< typename T >
	detail::q_exception_info< T >* get_info( )
	{
		for ( auto& info : infos( ) )
		{
			auto ei = detail::q_exception_info< T >::cast( *info );
			if ( ei )
				return ei;
		}
		return nullptr;
	}

	const std::vector< std::shared_ptr< detail::exception_info_base > >&
	infos( ) const;

	template< typename T >
	typename std::enable_if<
		!is_c_string< T >::value,
		exception&
	>::type
	operator<<( T&& t )
	{
		typedef typename std::decay< T >::type element_type;
		typedef detail::q_exception_info< element_type > derived;

		add_info( std::make_shared< derived >(
			std::forward< T >( t ) ) );

		return *this;
	}

	template< typename T >
	typename std::enable_if< is_c_string< T >::value, exception& >::type
	operator<<( T&& t )
	{
		return operator<<( std::string( std::forward< T >( t ) ) );
	}

	virtual const char* name( ) const noexcept
	{
		return "q::exception";
	}

private:
	void add_info( std::shared_ptr< detail::exception_info_base >&& ptr );

	std::vector< std::shared_ptr< detail::exception_info_base > >&
	infos( );

	struct pimpl;
	std::shared_ptr< pimpl > pimpl_;
};

template<
	typename E,
	bool Value =
		std::is_same<
			::q::exception,
			typename std::decay< E >::type
		>::value
		or
		std::is_base_of<
			::q::exception,
			typename std::decay< E >::type
		>::value
>
struct is_q_exception
: bool_type< Value >
{ };

std::ostream& operator<<( std::ostream&, const exception& );

namespace detail {

template< typename E >
static void _add_exception_properties( E& e )
{ }

template< typename E, typename Arg, typename... Args >
static void _add_exception_properties( E& e, Arg&& first, Args&&... args )
{
	e << std::forward< Arg >( first );

	_add_exception_properties( e, std::forward< Args >( args )... );
}

} // namespace detail

template< typename E, typename... Args >
static typename std::enable_if<
	is_q_exception< E >::value
	and
	std::is_rvalue_reference< E&& >::value,
	E
>::type
add_exception_properties( E&& e, Args&&... args )
{
	detail::_add_exception_properties( e, std::forward< Args >( args )... );

	return e;
}

template< typename E, typename... Args >
static typename std::enable_if<
	is_q_exception< E >::value
	and
	std::is_rvalue_reference< E&& >::value,
	E
>::type
add_exception_properties_with_stacktrace( E&& e, Args&&... args )
{
	detail::_add_exception_properties(
		e, get_stacktrace( ), std::forward< Args >( args )... );

	return e;
}

template< typename E, typename... Args >
static typename std::enable_if<
	!is_q_exception< E >::value
	and
	std::is_rvalue_reference< E&& >::value,
	E
>::type
add_exception_properties( E&& e, Args&&... args )
{
	return e;
}

template< typename E, typename... Args >
static typename std::enable_if<
	!is_q_exception< E >::value
	and
	std::is_rvalue_reference< E&& >::value,
	E
>::type
add_exception_properties_with_stacktrace( E&& e, Args&&... args )
{
	return e;
}

class stream_exception
{
public:
	stream_exception( ) = delete;
	stream_exception( std::exception_ptr&& e );
	stream_exception( const std::exception_ptr& e );

	const std::exception_ptr& exception( ) const;

private:
	std::exception_ptr exception_;
};

std::ostream& operator<<( std::ostream& , const stream_exception& );

Q_MAKE_STANDARD_EXCEPTION( logic_error );
	Q_MAKE_STANDARD_EXCEPTION( invalid_argument );
	Q_MAKE_STANDARD_EXCEPTION( domain_error );
	Q_MAKE_STANDARD_EXCEPTION( length_error );
	Q_MAKE_STANDARD_EXCEPTION( out_of_range );
Q_MAKE_STANDARD_EXCEPTION( runtime_error );
	Q_MAKE_STANDARD_EXCEPTION( range_error );
	Q_MAKE_STANDARD_EXCEPTION( overflow_error );
	Q_MAKE_STANDARD_EXCEPTION( underflow_error );
	Q_MAKE_STANDARD_EXCEPTION( system_error );
		Q_MAKE_STANDARD_EXCEPTION_OF_BASE(
			ios_base_failure, ::std::ios_base::failure );
Q_MAKE_STANDARD_EXCEPTION( bad_function_call );
Q_MAKE_STANDARD_EXCEPTION( bad_alloc );
#if defined( __GNUC__ ) && ( __GNUC__ < 5 ) && ( __GNUC_MINOR__ < 9 )
	// bad_array_new_length doesn't exist in GCC 4.8
#else
	Q_MAKE_STANDARD_EXCEPTION( bad_array_new_length );
#endif
Q_MAKE_STANDARD_EXCEPTION( bad_exception );

/**
 * Tries to convert an `exception_ptr` to a `std::string`, by
 * extracting exception information, using `q::stream_exception`;
 */
static inline std::string to_string( std::exception_ptr e )
{
	std::stringstream ss;
	ss << stream_exception( e );
	return ss.str( );
}

} // namespace q

#endif // LIBQ_EXCEPTION_EXCEPTION_HPP
