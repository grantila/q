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

#include <q/types.hpp>
#include <q/type_traits.hpp>

#include <exception>
#include <vector>
#include <iosfwd>
#include <sstream>

#define Q_THROW( ... ) \
	throw ::q::add_exception_properties( __VA_ARGS__ )

#define Q_MAKE_SIMPLE_EXCEPTION( Name ) \
	class Name \
	: public ::q::exception \
	{ \
		virtual const char* name( ) const noexcept override \
		{ \
			return #Name ; \
		} \
		using exception::exception; \
	}

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

template< typename T > class exception_info;

class exception_info_base
{
public:
	virtual ~exception_info_base( )
	{ }

	template< typename T >
	const exception_info< T >* cast( ) const
	{
		return dynamic_cast< const exception_info< T >* >( this );
	}

	template< typename T >
	exception_info< T >* cast( )
	{
		return dynamic_cast< exception_info< T >* >( this );
	}

	virtual std::string string( ) const = 0;

protected:
	exception_info_base( )
	{ }
};

template< typename T >
class exception_info
: public exception_info_base
{
public:
	exception_info( ) = delete;
	exception_info( const exception_info& ) = delete;

	exception_info( exception_info&& ) = default;

	exception_info( T&& t )
	: t_( std::move( t ) )
	{ }

	exception_info( const T& t )
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

	static exception_info< T >* cast( exception_info_base& eib )
	{
		return eib.cast< T >( );
	}

	static const exception_info< T >* cast( const exception_info_base& eib )
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
	exception( exception&& );
	exception( const exception& );
	virtual ~exception( );

	template< typename T >
	const detail::exception_info< T >* get_info( ) const
	{
		auto& _infos = infos( );
		for ( const auto& info : _infos )
		{
			auto ei = detail::exception_info< T >::cast( *info );
			if ( ei )
				return ei;
		}
		return nullptr;
	}

	template< typename T >
	detail::exception_info< T >* get_info( )
	{
		for ( auto& info : infos( ) )
		{
			auto ei = detail::exception_info< T >::cast( *info );
			if ( ei )
				return ei;
		}
		return nullptr;
	}

	const std::vector< std::shared_ptr< detail::exception_info_base > >&
	infos( ) const;

	template< typename T >
	typename std::enable_if<
		!std::is_lvalue_reference< T >::value and
		!is_c_string< T >::value,
		exception&
	>::type
	operator<<( T&& t )
	{
		std::shared_ptr< detail::exception_info_base > ptr(
			new detail::exception_info< T >(
				std::forward< T >( t ) ) );

		add_info( std::move( ptr ) );

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
	std::unique_ptr< pimpl > pimpl_;
};

std::ostream& operator<<( std::ostream&, const exception& );

template< typename E, typename Arg, typename... Args >
static typename std::enable_if<
	std::is_rvalue_reference< E&& >::value,
	E
>::type
add_exception_properties( E&& e, Arg&& first, Args&&... args )
{
	e << std::forward< Arg >( first );

	return add_exception_properties(
		std::forward< E >( e ),
		std::forward< Args >( args )... );
}

template< typename E >
static typename std::enable_if<
	std::is_rvalue_reference< E&& >::value,
	E
>::type
add_exception_properties( E&& e )
{
	return std::move( e );
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

} // namespace q

#endif // LIBQ_EXCEPTION_EXCEPTION_HPP
