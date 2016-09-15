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

#ifndef LIBQ_TEMPORARILY_COPYABLE_HPP
#define LIBQ_TEMPORARILY_COPYABLE_HPP

#include <type_traits>

/**
 * This function *forwards* value into a q::temporarily_copyable.
 */
#define Q_TEMPORARILY_COPYABLE( value ) \
	::q::make_temporarily_copyable( Q_FORWARD( value ) )

/**
 * This function *moves* value into a q::temporarily_copyable.
 */
#define Q_MOVE_TEMPORARILY_COPYABLE( value ) \
	::q::make_temporarily_copyable( std::move( value ) )

#ifndef LIBQ_WITH_CPP14
	/**
	 * Make a variable "movable" into a lambda.
	 * NOTE: This will move the variable internally, so it must not be used
	 * again, except with a Q_MOVABLE_MOVE or a Q_MOVABLE_CONSUME!
	 */
#	define Q_MAKE_MOVABLE( x ) \
		auto __movable_##x = Q_TEMPORARILY_COPYABLE( x )

	/**
	 * Make a variable movable into a lambda. Unlike Q_MAKE_MOVABLE, this
	 * will force-move the variable into this wrapper.
	 */
#	define Q_MOVE_INTO_MOVABLE( x ) \
		auto __movable_##x = Q_MOVE_TEMPORARILY_COPYABLE( x )

	/**
	 * Moves the variable into a lambda. Use this in the capture list.
	 */
#	define Q_MOVABLE_MOVE( x ) __movable_##x

	/**
	 * Moves the variable back from its internal wrapper, into a new
	 * variable or in a function argument.
	 */
#	define Q_MOVABLE_CONSUME( x ) ( std::move( __movable_##x.consume( ) ) )
#	define Q_MOVABLE_GET( x ) ( __movable_##x.get( ) )
#else
#	define Q_MAKE_MOVABLE( x )
#	define Q_MOVE_INTO_MOVABLE( x )
#	define Q_MOVABLE_MOVE( x ) x{ Q_FORWARD( x ) }
#	define Q_MOVABLE_CONSUME( x ) ( std::move( x ) )
#	define Q_MOVABLE_GET( x ) ( x )
#endif


namespace q {

template<
	typename T,
	typename Decay = typename std::remove_reference< T >::type,
	bool IsRvRef = std::is_rvalue_reference< T >::value,
	bool IsLvRef = std::is_lvalue_reference< T >::value,
	bool Copyable = std::is_copy_constructible< T >::value
>
class temporarily_copyable;

template< typename _, typename T, bool IsRvRef, bool IsLvRef >
class temporarily_copyable< _, T, IsRvRef, IsLvRef, true >
{
	temporarily_copyable( ) = delete;
	temporarily_copyable& operator=( const temporarily_copyable& ) = delete;
	temporarily_copyable& operator=( temporarily_copyable&& ) = delete;
public:
	typedef temporarily_copyable< _, T, IsRvRef, IsLvRef, true > this_type;

	temporarily_copyable( this_type&& ref ) = default;
	temporarily_copyable( const this_type& ref )
	: t_( std::move( const_cast< this_type& >( ref ).t_ ) )
	{ }

	temporarily_copyable( T&& t )
	: t_( std::move( t ) )
	{ }

	temporarily_copyable( const T& t )
	: t_( t )
	{ }

	T& get( )
	{
		return t_;
	}

	const T& get( ) const
	{
		return t_;
	}

	T consume( )
	{
		return std::move( t_ );
	}

private:
	T t_;
};

template< typename _, typename T, bool IsRvRef, bool IsLvRef >
class temporarily_copyable< _, T, IsRvRef, IsLvRef, false >
{
	temporarily_copyable( ) = delete;
	temporarily_copyable& operator=( const temporarily_copyable& ) = delete;
	temporarily_copyable& operator=( temporarily_copyable&& ) = delete;
public:
	typedef temporarily_copyable< _, T, IsRvRef, IsLvRef, false > this_type;

	temporarily_copyable( T&& t )
	: t_( std::move( t ) )
	{ }

	temporarily_copyable( this_type&& ref ) = default;
	temporarily_copyable( const this_type& ref )
	: t_( std::move( const_cast< this_type& >( ref ).t_ ) )
	{ }

	T& get( )
	{
		return t_;
	}

	const T& get( ) const
	{
		return t_;
	}

	T consume( )
	{
		return std::move( t_ );
	}

private:
	T t_;
};

template< typename T >
temporarily_copyable< typename std::decay< T >::type >
make_temporarily_copyable( T&& t )
{
	typedef temporarily_copyable<
		typename std::decay< T >::type
	> temporary_type;

	return temporary_type( std::forward< T >( t ) );
}

} // namespace q

#endif // LIBQ_TEMPORARILY_COPYABLE_HPP
