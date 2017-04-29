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

#ifndef LIBQ_TEST_BACKENDS_CPPUNIT_HPP
#define LIBQ_TEST_BACKENDS_CPPUNIT_HPP

#include <cppunit/extensions/HelperMacros.h>

#include <sstream>
#include <q/exception.hpp>

#define QTEST_BACKEND_REQUIRES_SINGLETON

#define QTEST_BACKEND_FAIL( ... ) do { \
	try \
	{ \
		CPPUNIT_FAIL( \
			( std::stringstream( ) << __VA_ARGS__ ).str( ) ); \
	} catch ( ... ) \
	{ \
		qtest_current_test.report( std::current_exception( ) ); \
	} \
} while ( false )


#define QTEST_BACKEND_FIXTURE_CLASS ::CppUnit::TestFixture
#define QTEST_BACKEND_FIXTURE_SETUP setUp
#define QTEST_BACKEND_FIXTURE_TEARDOWN tearDown
#define QTEST_BACKEND_FIXTURE_SETUP_TEARDOWN_ACCESS public


#ifdef QTEST_CREATE_MAIN

#include <iostream>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

int main( int argc, char* argv[ ] )
{
	q::settings settings;
	settings.set_long_stack_support( true );
	auto scope = q::scoped_initialize( settings );

#ifdef QTEST_CUSTOM_MAIN
	return qtest_custom_main( argc, argv );
#else
	CppUnit::TextUi::TestRunner runner;
	auto& registry = CppUnit::TestFactoryRegistry::getRegistry( );
	CppUnit::Test* suite = registry.makeTest( );
	runner.addTest( suite );
	auto outputter =
		new CppUnit::CompilerOutputter( &runner.result( ), std::cerr );

	runner.setOutputter( outputter );
	const bool wasSucessful = runner.run( );

	return wasSucessful ? 0 : 1;
#endif // QTEST_CUSTOM_MAIN
}

#endif // QTEST_CREATE_MAIN

#endif // LIBQ_TEST_BACKENDS_CPPUNIT_HPP
