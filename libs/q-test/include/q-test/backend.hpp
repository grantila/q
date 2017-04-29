/*
 * Copyright 2017 Gustaf Räntilä
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

#ifndef LIBQ_TEST_BACKEND_HPP
#define LIBQ_TEST_BACKEND_HPP

#ifdef QTEST_ON_GTEST
#	include <q-test/backends/gtest.hpp>
#elif defined( QTEST_ON_CATCH )
#	include <q-test/backends/catch.hpp>
#elif defined( QTEST_ON_BOOST ) || defined( QTEST_ON_BOOST_SH )
#	include <q-test/backends/boost.hpp>
#elif defined( QTEST_ON_DOCTEST )
#	include <q-test/backends/doctest.hpp>
#elif defined( QTEST_ON_CPPUNIT )
#	include <q-test/backends/cppunit.hpp>
#else
#	error No backend specified
	// Before including q-test headers, a backend must be defined
#endif


// Defaults:

#ifndef QTEST_BACKEND_FAIL_AT
#	define QTEST_BACKEND_FAIL_AT( file, line, ... ) \
		QTEST_BACKEND_FAIL( \
			file << ":" << line << LIBQ_EOL << __VA_ARGS__ )
#endif // QTEST_BACKEND_FAIL_AT


#ifndef QTEST_BACKEND_FIXTURE_SETUP_TEARDOWN_ACCESS
#	define QTEST_BACKEND_FIXTURE_SETUP_TEARDOWN_ACCESS protected
#endif

#ifdef QTEST_BACKEND_REQUIRES_SINGLETON
struct qtest_current_test_singleton
{
public:
	typedef std::function< void( std::exception_ptr ) > reporter_type;

	void set_reporter( reporter_type reporter )
	{
		reporter_ = std::move( reporter );
	}

	void unset_reporter( )
	{
		reporter_ = reporter_type( );
	}

	void report( std::exception_ptr e )
	{
		if ( reporter_ )
			return reporter_( e );

		std::cerr
			<< "Unit test threw exception outside its scope:"
			<< std::endl
			<< q::stream_exception( e );
		std::abort( );
	}

private:
	std::function< void( std::exception_ptr ) > reporter_;
};
static qtest_current_test_singleton qtest_current_test;
#endif // QTEST_BACKEND_REQUIRES_SINGLETON


#endif // LIBQ_TEST_BACKEND_HPP
