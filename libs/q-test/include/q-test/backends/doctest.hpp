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

#ifndef LIBQ_TEST_BACKENDS_DOCTEST_HPP
#define LIBQ_TEST_BACKENDS_DOCTEST_HPP

#ifdef QTEST_CREATE_MAIN
#	define DOCTEST_CONFIG_IMPLEMENT
#endif // QTEST_CREATE_MAIN

#include <doctest.h>

#if DOCTEST_VERSION_MAJOR > 1
#	define QTEST_BACKEND_FAIL( ... ) \
		FAIL( __VA_ARGS__ )
#	define QTEST_BACKEND_FAIL_AT( file, line, ... ) \
		ADD_FAIL_AT( file, line, __VA_ARGS__ )
#else
#	include <sstream>
#	include <iostream>
	static inline void qtest_doctest1_fail( std::stringstream ss )
	{
		std::cerr << ss.str( ) << std::endl;
		CHECK( false );
	}
#	define QTEST_BACKEND_FAIL( ... ) \
		qtest_doctest1_fail( std::stringstream( ) << __VA_ARGS__ )
#endif // DOCTEST_VERSION_MAJOR

#ifdef QTEST_CREATE_MAIN

int main( int argc, char* argv[ ] )
{
	q::settings settings;
	settings.set_long_stack_support( true );
	auto scope = q::scoped_initialize( settings );

#ifdef QTEST_CUSTOM_MAIN
	return qtest_custom_main( argc, argv );
#else
	return doctest::Context( argc, argv ).run( );
#endif // QTEST_CUSTOM_MAIN
}

#endif // QTEST_CREATE_MAIN

#endif // LIBQ_TEST_BACKENDS_DOCTEST_HPP
