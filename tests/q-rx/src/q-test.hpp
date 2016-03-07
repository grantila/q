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

#ifndef LIBQ_TEST_QTEST_HPP
#define LIBQ_TEST_QTEST_HPP

#include <q/pp.hpp>
#include <q/type_traits.hpp>
#include <q/functional.hpp>

#ifdef LIBQ_ON_GCC
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wsign-compare"
#endif // LIBQ_ON_GCC

#include <gtest/gtest.h>

#ifdef LIBQ_ON_GCC
#	pragma GCC diagnostic pop
#endif // LIBQ_ON_GCC


#define EXPECT_CALL( spy, ... ) \
	spy.expect_call( __FILE__, __LINE__, 1 ).template create< __VA_ARGS__ >

#define EXPECT_NO_CALL( spy, ... ) \
	spy.expect_call( __FILE__, __LINE__, 0 ).template create< __VA_ARGS__ >

#define EXPECT_N_CALLS( n, spy, ... ) \
	spy.expect_call( __FILE__, __LINE__, n ).template create< __VA_ARGS__ >

#define EXPECT_CALL_WRAPPER( spy ) \
	spy.expect_call( __FILE__, __LINE__, 1 ).wrap

#define EXPECT_NO_CALL_WRAPPER( spy ) \
	spy.expect_call( __FILE__, __LINE__, 0 ).wrap

#define EXPECT_N_CALLS_WRAPPER( n, spy ) \
	spy.expect_call( __FILE__, __LINE__, n ).wrap


namespace qtest {

class call_spy_counter
{
public:
	call_spy_counter( size_t expected_calls, const char* file, size_t line )
	: expected_calls_( expected_calls )
	, called_( 0 )
	, filename_( file )
	, line_( line )
	{ }

	size_t expected_calls( ) const
	{
		return expected_calls_;
	}

	size_t calls( ) const
	{
		return called_.load( );
	}

	bool ok( ) const
	{
		return calls( ) == expected_calls( );
	}

	void inc( )
	{
		++called_;
	}

	const char* file( ) const
	{
		return filename_;
	}

	size_t line( ) const
	{
		return line_;
	}

private:
	size_t expected_calls_;
	std::atomic< size_t > called_;
	const char* filename_;
	size_t line_;
};

template< typename Ret, typename... Args >
class call_spy
{
public:
	call_spy(
		const std::shared_ptr< call_spy_counter >& counter,
		Ret value
	)
	: counter_( counter )
	, value_( std::forward< Ret >( value ) )
	{ }

	typename q::remove_rvalue_reference< Ret >::type
	operator( )( Args... args )
	{
		counter_->inc( );
		return value_;
	}

private:
	std::shared_ptr< call_spy_counter > counter_;
	typename q::remove_rvalue_reference< Ret >::type value_;
};

template< typename... Args >
class call_spy< void, Args... >
{
public:
	call_spy( const std::shared_ptr< call_spy_counter >& counter )
	: counter_( counter )
	{ }

	void operator( )( Args... args )
	{
		counter_->inc( );
	}

private:
	std::shared_ptr< call_spy_counter > counter_;
};

template< typename Ret, typename... Args >
class call_spy_wrapper;

template< typename Ret, typename... Args >
class call_spy_wrapper< Ret, q::arguments< Args... > >
{
public:
	template< typename Fn >
	call_spy_wrapper(
		const std::shared_ptr< call_spy_counter >& counter,
		Fn&& fn
	)
	: counter_( counter )
	, fn_( std::forward< Fn >( fn ) )
	{ }

	Ret operator( )( Args... args )
	{
		counter_->inc( );
		return fn_( std::forward< Args >( args )... );
	}

private:
	std::shared_ptr< call_spy_counter > counter_;
	std::function< Ret( Args... ) > fn_;
};

class spy_maker
{
public:
	spy_maker(
		const std::shared_ptr< call_spy_counter >& counter,
		size_t calls
	)
	: counter_( counter )
	, calls_( calls )
	{ }

	template< typename Ret, typename... Args >
	typename std::enable_if<
		!std::is_void< Ret >::value,
		call_spy< Ret, Args... >
	>::type
	create( Ret&& ret )
	{
		typedef call_spy< Ret, Args... > return_type;

		return return_type( counter_, std::forward< Ret >( ret ) );
	}

	template< typename Ret, typename... Args >
	typename std::enable_if<
		std::is_void< Ret >::value,
		call_spy< Ret, Args... >
	>::type
	create( )
	{
		typedef call_spy< Ret, Args... > return_type;

		return return_type( counter_ );
	}

	template< typename Fn >
	call_spy_wrapper<
		Q_RESULT_OF( Fn ),
		Q_ARGUMENTS_OF( Fn )
	>
	wrap( Fn&& fn )
	{
		typedef call_spy_wrapper<
			Q_RESULT_OF( Fn ),
			Q_ARGUMENTS_OF( Fn )
		> return_type;

		return return_type( counter_, std::forward< Fn >( fn ) );
	}

private:
	std::shared_ptr< call_spy_counter > counter_;
	size_t calls_;
};

class spy
{
public:
	spy( )
	{ }

	virtual ~spy( )
	{
		for ( auto spy : spies_ )
		{
			if ( spy->ok( ) )
				continue;

			ADD_FAILURE_AT( spy->file( ), spy->line( ) )
				<< "Expected " << spy->expected_calls( )
				<< " calls, got " << spy->calls( );
		}
	}

	spy_maker expect_call( const char* filename, size_t line, size_t calls )
	{
		auto spy = std::make_shared< call_spy_counter >(
			calls, filename, line );

		spies_.push_back( spy );

		return spy_maker( spy, calls );
	}

private:
	std::vector< std::shared_ptr< call_spy_counter > > spies_;
};

} // namespace qtest

#endif // LIBQ_TEST_QTEST_HPP
