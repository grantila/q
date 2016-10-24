/*
 * Copyright 2016 Gustaf Räntilä
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

#ifndef LIBQ_TIME_SET_HPP
#define LIBQ_TIME_SET_HPP

#include <q/timer.hpp>

#include <map>

namespace q {

namespace detail {

template< typename T >
struct default_construction
{
	typedef T type;

	static type empty( )
	{
		return T( );
	}

	static type value( const T& t )
	{
		return t;
	}

	static type value( T&& t )
	{
		return t;
	}
};

template< typename T >
struct time_set_less_than
{
	typedef std::tuple< timer::point_type, T > element_type;

	constexpr bool operator( )(
		const element_type& a, const element_type& b ) const
	{
		return std::get< 0 >( a ) < std::get< 0 >( b );
	}
};

} // namespace detail

/**
 * The time_set class combines time and data of type T, ordered by time.
 *
 * This is useful to order data by time, and quickly be able to find the
 * closest data by time, as well as to find how long until the next time
 * (closest to now).
 */
template<
	typename T,
	typename LessThan = detail::time_set_less_than< T >,
	typename IfEmpty = detail::default_construction< T >
>
class time_set
{
public:
	typedef std::pair< timer::point_type, T > element_type;

	static_assert(
		!std::is_reference< T >::value, "References not allowed" );

	void push( timer::point_type time, T t )
	{
		element_type elem{ std::move( time ), std::move( t ) };
		map_.insert( std::move( elem ) );

		bool b = 1;
	}

	bool exists_before_or_at(
		timer::point_type time = timer::point_type::clock::now( ) )
	{
		if ( empty( ) )
			return false;

		return !( map_.begin( )->first > time );
	}

	typename IfEmpty::type pop( )
	{
		auto iter = map_.begin( );

		if ( iter == map_.end( ) )
			return IfEmpty::empty( );

		T value = std::move( iter->second );

		map_.erase( iter );

		return IfEmpty::value( std::move( value ) );
	}

	timer::duration_type next_time( )
	{
		auto iter = map_.begin( );

		if ( iter == map_.end( ) )
			timer::duration_type::max( );

		return iter->first - timer::point_type::clock::now( );
	}

	bool empty( ) const
	{
		return map_.empty( );
	}

private:
	std::multimap< timer::point_type, T > map_;
};

} // namespace q

#endif // LIBQ_TIME_SET_HPP
