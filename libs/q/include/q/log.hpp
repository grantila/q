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

#ifndef LIBQ_LOG_HPP
#define LIBQ_LOG_HPP

//#include <q/functional.hpp>

#include <q/types.hpp>

#include <iostream>

// TODO: We need some kind of way of visitor pattern where the entire logging
// is done in the user application, rather than stringifying here.

namespace q {

#define Q_LOG_CHAIN( ... ) \
	log_chain_generator( Q_HERE, Q_LOGTYPE_ADAPTER_CONSTRUCTOR( __VA_ARGS__ ) )

#define Q_LOG( ... ) \
	logstream( Q_HERE, Q_LOGTYPE_ADAPTER_CONSTRUCTOR( __VA_ARGS__ ) )

#ifndef Q_LOGTYPE_ADAPTER_CONSTRUCTOR
#	define Q_LOGTYPE_ADAPTER_CONSTRUCTOR( ... ) \
		::q::make_logtype_adapter( __VA_ARGS__ )
#endif

// TODO: Move these two
template<
	typename T,
	bool Is = std::is_same<
		typename std::decay<
			decltype( operator<<( std::stringstream( ), T( ) ) )
		>::type,
		std::ostream
	>::value
>
struct is_streamable
: std::false_type
{ };

template< typename T >
struct is_streamable< T, true >
: std::true_type
{ };

namespace detail {

template<
	typename Tuple,
	bool Streamable = is_streamable< Tuple >::value,
	typename... T
>
struct logtype_stringify
{
	std::string string( const Tuple& data )
	{
		std::stringstream ss;
		ss << data;
		return ss.str( );
	}
};

template< typename Tuple, typename... T >
struct logtype_stringify< Tuple, false, T... >
{
	template< typename A, typename... B >
	struct call_with_args
	{
		static std::string string( const A& a, const B&... b )
		{
			return logtype_stringify< A >::string( a ) +
				" " +
				call_with_args< B... >::string( b... );
		}
	};

	template< typename A >
	struct call_with_args< A >
	{
		static std::string string( const A& a )
		{
			return logtype_stringify< A >::string( a );
		}
	};

	template< typename A, typename... B >
	struct call_with_args< std::tuple< A, B... > >
	{
		static std::string string( const std::tuple< A, B... >& tuple )
		{
			return call_with_args_by_tuple( call_with_args< A, B... >::string, tuple );
		}
	};

	std::string string( const Tuple& data )
	{
		return call_with_args< Tuple >::string( data );
	}
};

} // namespace detail

template< typename... T >
struct logtype_stringify
{
	typedef std::tuple< T... > tuple_type;

	std::string string( const tuple_type& data )
	{
		return detail::logtype_stringify<
			tuple_type, is_streamable< tuple_type >::value, T...
		>::string( data );
	}
};

template< typename T >
struct logtype_stringify< T >
{
	static std::string string( const T& data )
	{
		// TODO: Enhance move-ness once C++ gets support for moving out a
		// string from a stringstream.
		std::stringstream ss;
		ss << data;
		return ss.str( );
	}
};


class logtype_adapter
{
public:
	virtual ~logtype_adapter( )
	{ }

	// Can be used to quickly map what this is a super class of, to do faster
	// lookup than dynamic_cast:ing this into subclasses.
	virtual std::ptrdiff_t id( ) const { return 0; }

	virtual std::string string( ) const = 0;
};

template< typename... T >
class any_logtype_adapter
: public logtype_adapter
{
public:
	any_logtype_adapter( T&&... t )
	: data_( std::forward_as_tuple( std::forward< T >( t )... ) )
	{ }

	std::string string( ) const override
	{
		return logtype_stringify< T... >::string( data_ );
	}

	std::tuple< T... > data_;
};

template< typename... T >
any_logtype_adapter< T... >
make_logtype_adapter( T&&... t )
{
	return any_logtype_adapter< T... >( std::forward< T >( t )... );
}

void register_logtype_adapter( );
void register_logger( );

// TODO: Move and possibly rename
template< typename Base, typename Derived >
std::unique_ptr< Base >
forward_as_unique( Derived&& derived )
{
	return std::unique_ptr< Base >(
		new Derived( std::forward< Derived >( derived ) )
	);
}

namespace detail {

struct perform_logging
{
	static void log( const macro_location& location,
	                 const std::unique_ptr< logtype_adapter >& adapter,
	                 const std::string& msg )
	{
		std::cout
			<< adapter->string( ) << " "
			<< location.string( ) << ": "
			<< msg;
	}
};

} // namespace detail

class logstream
//: public std::ostream
{
public:
	template< typename LogtypeAdapter >
	logstream( macro_location location,
	           LogtypeAdapter&& adapter,
	           typename std::enable_if<
		           std::is_base_of<
			           logtype_adapter,
			           LogtypeAdapter
		           >::value,
		           bool
	           >::type = false )
	: location_( location )
	, adapter_(
		forward_as_unique< logtype_adapter >(
			std::forward< LogtypeAdapter >( adapter )
		)
	)
	{ }

	~logstream( )
	{
		detail::perform_logging::log( location_, adapter_, "ostream..." );
	}

private:
	macro_location location_;
	std::unique_ptr< logtype_adapter > adapter_;
};

class log_chain_generator
{
public:
	enum class method
	{
		normal = 0,
	};

	log_chain_generator( )
	: method_( method::normal )
	{ }

	template< typename LogtypeAdapter >
	log_chain_generator( macro_location location,
	                     LogtypeAdapter&& adapter,
	                     typename std::enable_if<
		                     std::is_base_of<
			                     logtype_adapter,
			                     LogtypeAdapter
		                     >::value,
		                     bool
	                     >::type = false )
	: location_( location )
	, adapter_(
		forward_as_unique< logtype_adapter >(
			std::forward< LogtypeAdapter >( adapter )
		)
	)
	, method_( method::normal )
	{ }

	template< typename Args >
	void
	log( const Args& args )
	{
		detail::perform_logging::log( location_, adapter_, "ostream..." );

		//return std::forward_as_tuple( std::forward< Args >( args )... );
	}

	log_chain_generator& set_method( method m )
	{
		method_ = m;
		return *this;
	}

private:
	macro_location location_;
	std::unique_ptr< logtype_adapter > adapter_;
	method method_;
};

} // namespace q

#endif // LIBQ_LOG_HPP
