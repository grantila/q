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

#ifndef LIBQ_TEST_BACKENDS_BOOST_HPP
#define LIBQ_TEST_BACKENDS_BOOST_HPP

#ifdef QTEST_CREATE_MAIN
#	ifndef BOOST_TEST_MODULE
#		define BOOST_TEST_MODULE q-test powered suite
#	endif
#	define BOOST_TEST_ALTERNATIVE_INIT_API
#endif // QTEST_CREATE_MAIN

#if defined( QTEST_ON_BOOST_SH )
#	include <boost/test/included/unit_test.hpp>
#else
#	include <boost/test/unit_test.hpp>
#endif

#define QTEST_BACKEND_FAIL( ... ) \
	BOOST_FAIL( __VA_ARGS__ )

#ifdef QTEST_CREATE_MAIN

int main( int argc, char* argv[ ] )
{
	q::settings settings;
	settings.set_long_stack_support( true );
	auto scope = q::scoped_initialize( settings );

#ifdef QTEST_CUSTOM_MAIN
	return qtest_custom_main( argc, argv );
#else
	return ::boost::unit_test::unit_test_main(
		&init_unit_test, argc, argv );
#endif // QTEST_CUSTOM_MAIN
}

#endif // QTEST_CREATE_MAIN

#endif // LIBQ_TEST_BACKENDS_BOOST_HPP
