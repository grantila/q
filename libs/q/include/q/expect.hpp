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

#ifndef LIBQ_EXPECT_HPP
#define LIBQ_EXPECT_HPP

#include <q/type_traits.hpp>

#include <utility>

namespace q {

namespace detail {

class expect_exception
{
public:
	bool has_exception( ) const
	{
		return e_ != std::exception_ptr();
	}

	std::exception_ptr exception( ) const
	{
		return e_;
	}

	void rethrow_on_exception( ) const
	{
		if ( has_exception( ) )
			std::rethrow_exception( e_ );
	}

protected:
	expect_exception( )
	{ }

	expect_exception( std::exception_ptr&& e )
	{
		std::swap( e_, e );
	}

	expect_exception( const std::exception_ptr& e )
	: e_( e )
	{ }

	expect_exception( expect_exception&& ref )
	{
		std::swap( e_, ref.e_ );
	}
	expect_exception( const expect_exception& ) = default;

	expect_exception& operator=( expect_exception&& ref )
	{
		std::swap( e_, ref.e_ );
		return *this;
	}
	expect_exception& operator=( const expect_exception& ) = default;

	std::exception_ptr e_;
};

template<
	typename T,
	bool Static =
		::q::is_nothrow_default_constructible< T >::value &&
		(
			::q::is_nothrow_copy_constructible< T >::value ||
			!::q::is_copy_constructible< T >::value
		)
>
class expect_value
{
protected:
	expect_value( )
	: t_( )
	{ }

	expect_value( T&& t )
	: t_( std::move( t ) )
	{ }

	expect_value( const T& t )
	: t_( t )
	{ }

	expect_value( expect_value< T, Static >&& ) = default;
	expect_value( const expect_value< T, Static >& ) = default;

	expect_value& operator=( expect_value< T, Static >&& ) = default;
	expect_value& operator=( const expect_value< T, Static >& ) = default;

	const T& _get( ) const
	{
		return t_;
	}

	T _consume( )
	{
		return std::move( t_ );
	}

	T t_;
};

template< typename T >
class expect_value< T, false >
{
protected:
	expect_value( )
	{ }

	expect_value( T&& t )
	: t_( nullptr )
	{
		// TODO: Handle exceptions (uncaught exception or similar)
		t_.reset( new T( std::move( t ) ) );
	}

	expect_value( const T& t )
	: t_( nullptr )
	{
		// TODO: Handle exceptions (uncaught exception or similar)
		t_.reset( new T( t ) );
	}

	expect_value( expect_value< T, false >&& ) = default;
	expect_value( const expect_value< T, false >& ) = default;

	expect_value& operator=( expect_value< T, false >&& ) = default;
	expect_value& operator=( const expect_value< T, false >& ) = default;

	const T& _get( ) const
	{
		return *t_;
	}

	T _consume( )
	{
		return std::move( *t_ );
	}

	std::unique_ptr< T > t_;
};

template< >
class expect_value< void >
{
protected:
	expect_value( ) = default;
	expect_value( const expect_value< void >& ) = default;
	expect_value( expect_value< void >&& ) = default;

	expect_value& operator=( expect_value< void >&& ) = default;
	expect_value& operator=( const expect_value< void >& ) = default;

	void _get( ) const
	{ }

	void _consume( )
	{ }
};

template< >
class expect_value< std::exception_ptr >
{
protected:
	expect_value( )
	: t_( nullptr )
	{ }

	expect_value( std::exception_ptr&& t )
	: t_( std::move( t ) )
	{ }

	expect_value( const std::exception_ptr& t )
	: t_( t )
	{ }

	expect_value( expect_value< std::exception_ptr >&& ) = default;
	expect_value( const expect_value< std::exception_ptr >& ) = default;

	expect_value& operator=( expect_value< std::exception_ptr >&& ) = default;
	expect_value& operator=( const expect_value< std::exception_ptr >& ) = default;

	const std::exception_ptr& _get( ) const
	{
		return t_;
	}

	std::exception_ptr _consume( )
	{
		return std::move( t_ );
	}

	std::exception_ptr t_;
};

} // detail

template<
	typename T,
	bool CopyConstructible = ::q::is_copy_constructible< T >::value
>
class expect
: public detail::expect_exception
, private detail::expect_value< T >
{
	typedef detail::expect_value< T > base;

public:
	typedef expect< T, CopyConstructible > self_type;

	expect( ) = default;

	expect( T&& t )
	: detail::expect_exception( nullptr )
	, base( std::move( t ) )
	{ }

	expect( const T& t )
	: detail::expect_exception( nullptr )
	, base( t )
	{ }

	explicit expect( std::exception_ptr&& e )
	: detail::expect_exception( std::move( e ) )
	{ }

	explicit expect( const std::exception_ptr& e )
	: detail::expect_exception( e )
	{ }

	expect( self_type&& ) = default;
	expect( const self_type& ) = default;

	self_type& operator=( self_type&& ) = default;
	self_type& operator=( const self_type& ) = default;

	const T& get( ) const
	{
		rethrow_on_exception( );
		return base::_get( );
	}

	T consume( )
	{
		rethrow_on_exception( );
		return std::move( base::_consume( ) );
	}
};

template< typename T >
class expect< T, false >
: public detail::expect_exception
, private detail::expect_value< T >
{
	typedef detail::expect_value< T > base;

public:
	typedef expect< T, false > self_type;

	expect( ) = default;

	expect( T&& t )
	: detail::expect_exception( nullptr )
	, base( std::move( t ) )
	{ }

	expect( const T& t )
	: detail::expect_exception( nullptr )
	, base( t )
	{ }

	explicit expect( std::exception_ptr&& e )
	: detail::expect_exception( std::move( e ) )
	{ }

	explicit expect( const std::exception_ptr& e )
	: detail::expect_exception( e )
	{ }

	expect( self_type&& ) = default;
	expect( const self_type& ) = delete;

	self_type& operator=( self_type&& ) = default;
	self_type& operator=( const self_type& ) = delete;

	const T& get( ) const
	{
		rethrow_on_exception( );
		return base::_get( );
	}

	T consume( )
	{
		rethrow_on_exception( );
		return std::move( base::_consume( ) );
	}
};

template< >
class expect< void, false >
: public detail::expect_exception
, private detail::expect_value< void >
{
	typedef detail::expect_value< void > base;

public:
	expect( )
	: detail::expect_exception( nullptr )
	{ }

	explicit expect( std::exception_ptr&& e )
	: detail::expect_exception( std::move( e ) )
	{ }

	explicit expect( const std::exception_ptr& e )
	: detail::expect_exception( e )
	{ }

	expect( expect< void, false >&& ) = default;
	expect( const expect< void, false >& ) = delete;

	expect& operator=( expect< void, false >&& ) = default;
	expect& operator=( const expect< void, false >& ) = default;

	void get( ) const
	{
		rethrow_on_exception( );
		return base::_get( );
	}

	void consume( )
	{
		rethrow_on_exception( );
		return base::_consume( );
	}
};

template< >
class expect< std::exception_ptr, true >
: public detail::expect_exception
, private detail::expect_value< std::exception_ptr >
{
	typedef detail::expect_value< std::exception_ptr > base;

public:
	expect( ) = default;

	expect( std::exception_ptr&& t, bool expected )
	: detail::expect_exception( expected ? std::move( t ) : nullptr )
	, base( expected ? nullptr : std::move( t ) )
	{ }

	expect( const std::exception_ptr& t, bool expected )
	: detail::expect_exception( expected ? t : nullptr )
	, base( expected ? nullptr : t )
	{ }

	expect( expect< std::exception_ptr >&& ) = default;
	expect( const expect< std::exception_ptr >& ) = default;

	expect& operator=( expect< std::exception_ptr, true >&& ) = default;
	expect& operator=( const expect< std::exception_ptr, true >& ) = default;

	const std::exception_ptr& get( ) const
	{
		rethrow_on_exception( );
		return base::_get( );
	}

	std::exception_ptr&& consume( )
	{
		rethrow_on_exception( );
		return std::move( base::_consume( ) );
	}
};

template< typename T >
typename std::enable_if<
	!is_same_type< void, T >::value &&
	!is_same_type< std::exception_ptr, T >::value,
	expect< T >
>::type
fulfill( T&& t )
{
	return expect< T >( std::forward< T >( t ) );
}

template< typename T >
typename std::enable_if<
	is_same_type< void, T >::value,
	expect< void >
>::type
fulfill( )
{
	return expect< void >( );
}

template< typename T, typename E >
typename std::enable_if<
	!is_same_type< std::exception_ptr, T >::value &&
	is_same_type< std::exception_ptr, E >::value,
	expect< T >
>::type
refuse( E&& e )
{
	return expect< T >( std::forward< E >( e ) );
}

template< typename T, typename E >
typename std::enable_if<
	is_same_type< std::exception_ptr, E >::value &&
	is_same_type< std::exception_ptr, T >::value,
	expect< T >
>::type
fulfill( E&& e )
{
	return expect< T >( std::forward< E >( e ), true );
}

template< typename T, typename E >
typename std::enable_if<
	is_same_type< std::exception_ptr, E >::value &&
	is_same_type< std::exception_ptr, T >::value,
	expect< T >
>::type
refuse( std::exception_ptr e )
{
	return expect< T >( std::forward< E >( e ), true );
}

} // namespace q

#endif // LIBQ_EXPECT_HPP
